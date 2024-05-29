#include <jwrite/ui/JustWrite.h>
#include <jwrite/ui/ScrollArea.h>
#include <jwrite/ui/BookInfoEdit.h>
#include <jwrite/ui/ColorSchemeDialog.h>
#include <jwrite/ui/MessageBox.h>
#include <jwrite/ui/ToolbarIcon.h>
#include <jwrite/ColorScheme.h>
#include <jwrite/AppConfig.h>
#include <jwrite/ProfileUtils.h>
#include <widget-kit/TextInputDialog.h>
#include <widget-kit/OverlaySurface.h>
#include <QScrollBar>
#include <QVBoxLayout>
#include <QToolTip>
#include <QKeyEvent>
#include <QDateTime>
#include <QPainter>
#include <QFileDialog>
#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QApplication>

using namespace widgetkit;

namespace jwrite::ui {

class BookManager : public InMemoryBookManager {
public:
    bool chapter_cached(int cid) const {
        return chapters_.contains(cid);
    }

    bool is_chapter_dirty(int cid) const {
        return modified_.contains(cid);
    }

    OptionalString fetch_chapter_content(int cid) override {
        if (!has_chapter(cid)) {
            return std::nullopt;
        } else if (chapters_.contains(cid)) {
            return {chapters_.value(cid)};
        } else if (const auto path = get_path_to_chapter(cid); QFile::exists(path)) {
            QFile file(path);
            file.open(QIODevice::ReadOnly | QIODevice::Text);
            Q_ASSERT(file.isOpen());
            const auto content = file.readAll();
            file.close();
            chapters_[cid] = content;
            return content;
        } else {
            return QString{};
        }
    }

    bool sync_chapter_content(int cid, const QString &text) override {
        if (!has_chapter(cid)) { return false; }
        chapters_[cid] = text;
        modified_.insert(cid);
        return true;
    }

    QString get_path_to_chapter(int cid) const {
        QDir dir{AppConfig::get_instance().path(AppConfig::StandardPath::UserData)};
        dir.cd(AbstractBookManager::info_ref().uuid);
        return dir.filePath(QString::number(cid));
    }

private:
    QMap<int, QString> chapters_;
    QSet<int>          modified_;
};

void JustWrite::request_create_new_book() {
    BookInfo book_info{.uuid = BookManager::alloc_uuid()};
    Q_ASSERT(!books_.contains(book_info.uuid));
    do_create_book(book_info);
    ui_gallery_->updateDisplayCaseItem(ui_gallery_->totalItems(), book_info);
    request_update_book_info(book_info.uuid);
}

void JustWrite::do_create_book(BookInfo &book_info) {
    Q_ASSERT(!books_.contains(book_info.uuid));

    book_info.title            = "未命名书籍";
    book_info.author           = get_default_author();
    book_info.cover_url        = ":/res/default-cover.png";
    book_info.creation_time    = QDateTime::currentDateTimeUtc();
    book_info.last_update_time = QDateTime::currentDateTimeUtc();

    auto bm        = new BookManager;
    bm->info_ref() = book_info;
    books_.insert(book_info.uuid, bm);
}

void JustWrite::request_remove_book(const QString &book_id) {
    Q_ASSERT(books_.contains(book_id));
    const auto choice = MessageBox::show(
        ui_surface_,
        "删除书籍",
        "删除后，作品将无法恢复，请谨慎操作。",
        MessageBox::StandardIcon::Warning);
    if (choice == MessageBox::Yes) {
        const int item_index = ui_gallery_->indexOf(book_id);
        Q_ASSERT(item_index != -1);
        ui_gallery_->removeDisplayCase(item_index);
        do_remove_book(book_id);
    }
}

void JustWrite::do_remove_book(const QString &book_id) {
    Q_ASSERT(books_.contains(book_id));
    auto bm = books_.take(book_id);
    delete bm;
}

void JustWrite::request_open_book(const QString &book_id) {}

void JustWrite::do_open_book(const QString &book_id) {}

void JustWrite::request_close_opened_book() {}

void JustWrite::do_close_book(const QString &book_id) {}

void JustWrite::request_update_book_info(const QString &book_id) {
    Q_ASSERT(books_.contains(book_id));
    const auto &initial = books_.value(book_id)->info_ref();
    if (auto opt = BookInfoEdit::getBookInfo(ui_surface_, initial)) {
        auto &book_info = *opt;
        Q_ASSERT(book_info.uuid == book_id);
        do_update_book_info(book_info);
        const int item_index = ui_gallery_->indexOf(book_id);
        Q_ASSERT(item_index != -1);
        ui_gallery_->updateDisplayCaseItem(item_index, book_info);
        set_default_author(book_info.author, false);
    }
}

void JustWrite::do_update_book_info(BookInfo &book_info) {
    Q_ASSERT(books_.contains(book_info.uuid));
    if (book_info.title.isEmpty()) { book_info.title = "未命名书籍"; }
    if (book_info.author.isEmpty()) { book_info.author = get_default_author(); }
    if (book_info.cover_url.isEmpty()) { book_info.cover_url = ":/res/default-cover.png"; }
    Q_ASSERT(book_info.creation_time.isValid());
    Q_ASSERT(book_info.last_update_time.isValid());
    books_.value(book_info.uuid)->info_ref() = book_info;
}

void JustWrite::request_rename_toc_item(const QString &book_id, int toc_id, TocType type) {
    Q_ASSERT(books_.contains(book_id));

    auto bm = books_.value(book_id);
    Q_ASSERT(bm->has_toc_item(toc_id));
    Q_ASSERT(type != TocType::Volume || bm->has_volume(toc_id));
    Q_ASSERT(type != TocType::Chapter || bm->has_chapter(toc_id));

    const auto caption = type == TocType::Volume ? "分卷名" : "章节名";
    const auto hint    = "请输入新标题";
    if (auto opt = TextInputDialog::getInputText(
            ui_surface_, bm->get_title(toc_id).value(), caption, hint)) {
        const auto &title = *opt;
        do_rename_toc_item(book_id, toc_id, title);
        //! FIXME: just notify the book dir to update
        ui_edit_page_->renameBookDirItem(toc_id, title);
    }
}

void JustWrite::do_rename_toc_item(const QString &book_id, int toc_id, const QString &title) {
    auto bm = books_.value(book_id);
    Q_ASSERT(bm->has_toc_item(toc_id));
    const bool succeed = bm->update_title(toc_id, title);
    Q_ASSERT(succeed);
}

void JustWrite::handle_gallery_on_click(int index) {
    if (index == ui_gallery_->totalItems()) { request_create_new_book(); }
}

void JustWrite::handle_gallery_on_menu_action(int index, Gallery::MenuAction action) {
    Q_ASSERT(index >= 0 && index < ui_gallery_->totalItems());
    const auto &book_id = ui_gallery_->bookInfoAt(index).uuid;
    switch (action) {
        case Gallery::Open: {
            requestStartEditBook(index);
        } break;
        case Gallery::Edit: {
            request_update_book_info(book_id);
        } break;
        case Gallery::Delete: {
            request_remove_book(book_id);
        } break;
    }
}

void JustWrite::handle_gallery_on_load_book(const BookInfo &book_info) {
    if (!books_.contains(book_info.uuid)) {
        auto bm        = new BookManager;
        bm->info_ref() = book_info;
        books_.insert(book_info.uuid, bm);
    }
    ui_gallery_->updateDisplayCaseItem(ui_gallery_->totalItems(), book_info);
}

void JustWrite::handle_book_dir_on_rename_toc_item(
    const QString &book_id, int toc_id, TocType type) {
    request_rename_toc_item(book_id, toc_id, type);
}

void JustWrite::handle_book_dir_on_rename_toc_item__adapter(
    const BookInfo &book_info, int vid, int cid) {
    const auto type   = cid == -1 ? TocType::Volume : TocType::Chapter;
    const auto toc_id = type == TocType::Volume ? vid : cid;
    handle_book_dir_on_rename_toc_item(book_info.uuid, toc_id, type);
}

void JustWrite::handle_on_page_change(PageType page) {
    switch (page) {
        case PageType::Gallery: {
            ui_title_bar_->setTitle("只写 丶 阐释你的梦");
        } break;
        case PageType::Edit: {
            const auto &info  = ui_edit_page_->bookSource().info_ref();
            const auto  title = QString("%1\u3000%2 [著]").arg(info.title).arg(info.author);
            ui_title_bar_->setTitle(title);
        } break;
    }
}

void JustWrite::handle_on_open_settings() {
    //! TODO: currently only support editing the color scheme, impl it later

    auto &config = AppConfig::get_instance();

    const auto old_theme  = config.theme();
    const auto old_scheme = config.scheme();

    auto dialog = std::make_unique<ColorSchemeDialog>(old_theme, old_scheme, this);

    connect(
        dialog.get(),
        &ColorSchemeDialog::applyRequested,
        this,
        [this, &config](ColorTheme theme, const ColorScheme &scheme) {
            config.set_theme(theme);
            config.set_scheme(scheme);
        });

    const int result = dialog->exec();

    if (result != QDialog::Accepted) {
        config.set_theme(old_theme);
        config.set_scheme(old_scheme);
        updateColorScheme(old_scheme);
    } else {
        config.set_theme(dialog->getTheme());
        config.set_scheme(dialog->getScheme());
        updateColorScheme(config.scheme());
    }
}

JustWrite::JustWrite() {
    init();
    setupUi();
    setupConnections();

    switchToPage(PageType::Gallery);

    //! NOTE: Qt gives out an unexpected minimum height to my widgets and QLayout::invalidate()
    //! could not solve the problem, maybe there is some dirty cached data at the bottom level.
    //! I eventually found that an explicit call to the expcted-to-be-readonly method sizeHint()
    //! could make effects on the confusing issue. so, **DO NOT TOUCH THE CODE** unless you
    //! figure out a better way to solve the problem
    const auto DO_NOT_REMOVE_THIS_STATEMENT = sizeHint();

    requestInitFromLocalStorage();

    command_manager_.load_default();
}

JustWrite::~JustWrite() {
    AppConfig::get_instance().save();

    for (auto book : books_) { delete book; }
    books_.clear();

    jwrite_profiler_dump(QString("jwrite-profiler.%1.log")
                             .arg(QDateTime::currentDateTimeUtc().toString("yyyyMMddHHmmss")));
}

void JustWrite::wait(std::function<void()> job) {
    Progress::wait(ui_surface_, job);
}

void JustWrite::updateColorScheme(const ColorScheme &scheme) {
    auto pal = palette();
    pal.setColor(QPalette::Window, scheme.window());
    pal.setColor(QPalette::WindowText, scheme.window_text());
    pal.setColor(QPalette::Base, scheme.text_base());
    pal.setColor(QPalette::Text, scheme.text());
    pal.setColor(QPalette::Highlight, scheme.selected_text());
    pal.setColor(QPalette::Button, scheme.window());
    pal.setColor(QPalette::ButtonText, scheme.window_text());
    setPalette(pal);

    if (auto w = static_cast<QScrollArea *>(page_map_[PageType::Gallery])->verticalScrollBar()) {
        auto pal = w->palette();
        pal.setColor(w->backgroundRole(), scheme.text_base());
        pal.setColor(w->foregroundRole(), scheme.window());
        w->setPalette(pal);
    }

    ui_title_bar_->updateColorScheme(scheme);
    ui_toolbar_->update_color_scheme(scheme);
    ui_gallery_->updateColorScheme(scheme);
    ui_edit_page_->updateColorScheme(scheme);

    auto font = this->font();
    font.setPointSize(10);
    QToolTip::setFont(font);

    pal.setColor(QPalette::ToolTipBase, scheme.window());
    pal.setColor(QPalette::ToolTipText, scheme.window_text());
    QToolTip::setPalette(pal);
}

QString JustWrite::get_default_author() const {
    return likely_author_.isEmpty() ? "佚名" : likely_author_;
}

void JustWrite::set_default_author(const QString &author, bool force) {
    if (likely_author_.isEmpty() || force) { likely_author_ = author; }
}

void JustWrite::toggleMaximize() {
    if (isMinimized()) { return; }
    if (isMaximized()) {
        showNormal();
    } else {
        showMaximized();
    }
}

void JustWrite::init() {
    action_goto_gallery_    = new QAction(this);
    action_goto_edit_page_  = new QAction(this);
    action_goto_favorites_  = new QAction(this);
    action_goto_trash_bin_  = new QAction(this);
    action_export_          = new QAction(this);
    action_share_           = new QAction(this);
    action_fullscreen_      = new QAction(this);
    action_exit_fullscreen_ = new QAction(this);
    action_show_help_       = new QAction(this);
    action_open_settings_   = new QAction(this);
}

void JustWrite::setupUi() {
    setObjectName("JustWrite");

    auto top_layout = new QVBoxLayout(this);

    auto container        = new QWidget;
    auto layout_container = new QHBoxLayout(container);

    ui_title_bar_  = new TitleBar;
    ui_toolbar_    = new Toolbar;
    ui_page_stack_ = new QStackedWidget;
    ui_gallery_    = new Gallery;
    ui_edit_page_  = new EditPage;

    auto gallery_page = new ScrollArea;
    gallery_page->setWidget(ui_gallery_);

    ui_page_stack_->addWidget(gallery_page);
    ui_page_stack_->addWidget(ui_edit_page_);

    layout_container->addWidget(ui_toolbar_);
    layout_container->addWidget(ui_page_stack_);

    top_layout->addWidget(ui_title_bar_);
    top_layout->addWidget(container);

    ui_toolbar_->add_item("书库", "toolbar/gallery", action_goto_gallery_, false);
    ui_toolbar_->add_item("编辑", "toolbar/draft", action_goto_edit_page_, false);
    ui_toolbar_->add_item("素材收藏", "toolbar/favorites", action_goto_favorites_, false);
    ui_toolbar_->add_item("回收站", "toolbar/trash", action_goto_trash_bin_, false);
    ui_toolbar_->add_item("导出", "toolbar/export", action_export_, false);
    ui_toolbar_->add_item("分享", "toolbar/share", action_share_, false);
    ui_toolbar_->add_item("全屏", "toolbar/fullscreen", action_fullscreen_, true);
    ui_toolbar_->add_item("帮助", "toolbar/help", action_show_help_, true);
    ui_toolbar_->add_item("设置", "toolbar/settings", action_open_settings_, true);

    ui_surface_        = new OverlaySurface;
    const bool succeed = ui_surface_->setup(container);
    Q_ASSERT(succeed);

    using AgentButton = QWK::WidgetWindowAgent::SystemButton;
    ui_agent_         = new QWK::WidgetWindowAgent(this);
    ui_agent_->setup(this);
    ui_agent_->setTitleBar(ui_title_bar_);
    ui_agent_->setSystemButton(AgentButton::Minimize, ui_title_bar_->minimize_button());
    ui_agent_->setSystemButton(AgentButton::Maximize, ui_title_bar_->maximize_button());
    ui_agent_->setSystemButton(AgentButton::Close, ui_title_bar_->close_button());

    layout_container->setContentsMargins({});
    layout_container->setSpacing(0);

    top_layout->setContentsMargins({});
    top_layout->setSpacing(0);

    page_map_[PageType::Gallery] = gallery_page;
    page_map_[PageType::Edit]    = ui_edit_page_;

    ui_tray_icon_ = new QSystemTrayIcon(this);
    ui_tray_icon_->setIcon(QIcon(":/app.ico"));
    ui_tray_icon_->setToolTip("只写");
    ui_tray_icon_->setVisible(false);

    updateColorScheme(AppConfig::get_instance().scheme());
}

void JustWrite::setupConnections() {
    auto &config = AppConfig::get_instance();

    connect(action_goto_gallery_, &QAction::triggered, this, [this] {
        if (current_page_ == PageType::Gallery) { return; }
        wait(std::bind(&EditPage::syncAndClearEditor, ui_edit_page_));
        switchToPage(PageType::Gallery);
    });
    connect(action_open_settings_, &QAction::triggered, this, &JustWrite::handle_on_open_settings);

    connect(&config, &AppConfig::on_theme_change, this, [this, &config] {
        updateColorScheme(config.scheme());
    });
    connect(&config, &AppConfig::on_scheme_change, this, &JustWrite::updateColorScheme);
    connect(ui_title_bar_, &TitleBar::minimizeRequested, this, &QWidget::showMinimized);
    connect(ui_title_bar_, &TitleBar::maximizeRequested, this, &JustWrite::toggleMaximize);
    connect(ui_title_bar_, &TitleBar::closeRequested, this, &JustWrite::closePage);
    connect(ui_gallery_, &Gallery::clicked, this, &JustWrite::handle_gallery_on_click);
    connect(ui_gallery_, &Gallery::menuClicked, this, &JustWrite::handle_gallery_on_menu_action);
    connect(
        ui_edit_page_,
        &EditPage::renameTocItemRequested,
        this,
        &JustWrite::handle_book_dir_on_rename_toc_item__adapter);
    connect(
        this,
        &JustWrite::pageChanged,
        this,
        &JustWrite::handle_on_page_change,
        Qt::QueuedConnection);
    connect(ui_edit_page_, &EditPage::quitEditRequested, action_goto_gallery_, &QAction::trigger);
    connect(
        ui_edit_page_, &EditPage::openSettingsRequested, action_open_settings_, &QAction::trigger);
}

void JustWrite::requestStartEditBook(int index) {
    const auto  book_info = ui_gallery_->bookInfoAt(index);
    const auto &uuid      = book_info.uuid;

    if (!books_.contains(uuid)) {
        auto bm        = new BookManager;
        bm->info_ref() = book_info;
        books_.insert(uuid, bm);
    }

    auto bm = books_.value(uuid);
    Q_ASSERT(bm);

    waitTaskBuilder()
        .withPolicy(Progress::UseMinimumDisplayTime, false)
        .withBlockingJob([this, bm] {
            ui_edit_page_->resetBookSource(bm);
            switchToPage(PageType::Edit);
        })
        .withAsyncJob([this, bm] {
            ui_edit_page_->resetWordsCount();
            ui_edit_page_->flushWordsCount();
            if (const auto &chapters = bm->get_all_chapters(); !chapters.isEmpty()) {
                ui_edit_page_->openChapter(chapters.back());
            }
            ui_edit_page_->editor()->prepareRenderData();
        })
        .exec(ui_surface_);

    ui_edit_page_->focusOnEditor();
    ui_edit_page_->syncWordsStatus();
}

void JustWrite::requestInitFromLocalStorage() {
    const QDir data_dir{AppConfig::get_instance().path(AppConfig::StandardPath::UserData)};
    if (data_dir.exists()) {
        loadDataFromLocalStorage();
    } else {
        initLocalStorage();
    }
}

void JustWrite::requestQuitApp() {
    wait(std::bind(&JustWrite::syncToLocalStorage, this));
    QCoreApplication::exit();
}

void JustWrite::initLocalStorage() {
    QDir dir{AppConfig::get_instance().path(AppConfig::StandardPath::UserData)};

    if (!dir.exists()) {
        const bool succeed = dir.mkdir(".");
        Q_ASSERT(succeed);
    }

    QJsonObject local_storage;
    local_storage["major_author"]     = QString{};
    local_storage["last_update_time"] = QDateTime::currentDateTimeUtc().toString();
    local_storage["data"]             = QJsonArray{};

    /*! Json Structure
     *  {
     *      "data": [
     *          {
     *              "book_id": "<book-uuid>",
     *              "name": "<name>",
     *              "author": "<author>",
     *              "cover_url": "<cover-url>",
     *              "last_update_time": "<last-update-time>"
     *          }
     *      ],
     *      "major_author" "<major-author>",
     *      "last_update_time": "<last-update-time>"
     *  }
     **/

    QFile data_file(dir.filePath("mainfest.json"));
    data_file.open(QIODevice::WriteOnly | QIODevice::Text);

    data_file.write(QJsonDocument(local_storage).toJson());

    data_file.close();
}

void JustWrite::loadDataFromLocalStorage() {
    QDir dir{AppConfig::get_instance().path(AppConfig::StandardPath::UserData)};
    Q_ASSERT(dir.exists());

    Q_ASSERT(dir.exists("mainfest.json"));

    QFile data_file(dir.filePath("mainfest.json"));
    data_file.open(QIODevice::ReadOnly | QIODevice::Text);
    const auto text = data_file.readAll();
    auto       json = QJsonDocument::fromJson(text);
    data_file.close();

    Q_ASSERT(json.isObject());
    const auto &local_storage = json.object();

    likely_author_        = local_storage["major_author"].toString("");
    const auto &book_data = local_storage["data"].toArray({});

    for (const auto &ref : book_data) {
        Q_ASSERT(ref.isObject());
        const auto &book = ref.toObject();
        const auto &uuid = book["book_id"].toString("");
        const auto &name = book["name"].toString("");
        Q_ASSERT(!uuid.isEmpty() && !name.isEmpty());

        BookInfo book_info{
            .uuid          = uuid,
            .title         = name,
            .author        = book["author"].toString(""),
            .cover_url     = book["cover_url"].toString(""),
            .creation_time = QDateTime::fromString(book["creation_time"].toString(""), Qt::ISODate),
            .last_update_time =
                QDateTime::fromString(book["last_update_time"].toString(""), Qt::ISODate),
        };

        if (!book_info.creation_time.isValid()) {
            book_info.creation_time = QDateTime::currentDateTimeUtc();
        }
        if (!book_info.last_update_time.isValid()) {
            book_info.last_update_time = QDateTime::currentDateTimeUtc();
        }

        handle_gallery_on_load_book(book_info);

        if (!dir.exists(uuid)) { continue; }

        const bool succeed = dir.cd(uuid);
        Q_ASSERT(succeed);

        auto bm = books_.value(uuid);
        Q_ASSERT(bm);

        if (dir.exists("TOC")) {
            const auto toc_path = dir.filePath("TOC");
            QFile      toc_file(toc_path);
            toc_file.open(QIODevice::ReadOnly | QIODevice::Text);
            const auto toc_text = toc_file.readAll();
            toc_file.close();

            const auto toc_json = QJsonDocument::fromJson(toc_text);
            Q_ASSERT(toc_json.isArray());
            const auto &volumes = toc_json.array();

            /*! Json Structure
             *  [
             *      {
             *          'vid': <volume-id>,
             *          'title': '<volume-title>',
             *          'chapters': [
             *              {
             *                  'cid': <chapter-id>,
             *                  'title': '<chapter-title>',
             *              }
             *          ]
             *      }
             *  ]
             */

            int vol_index = 0;
            for (const auto &vol_ref : volumes) {
                Q_ASSERT(vol_ref.isObject());
                const auto &volume    = vol_ref.toObject();
                const auto  vid       = volume["vid"].toInt();
                const auto  vol_title = volume["title"].toString("");
                bm->add_volume_as(vol_index++, vid, vol_title);
                const auto &chapters   = volume["chapters"].toArray({});
                int         chap_index = 0;
                for (const auto &chap_ref : chapters) {
                    Q_ASSERT(chap_ref.isObject());
                    const auto &chapter    = chap_ref.toObject();
                    const auto  cid        = chapter["cid"].toInt();
                    const auto  chap_title = chapter["title"].toString("");
                    bm->add_chapter_as(vid, chap_index++, cid, chap_title);
                }
            }
        }

        dir.cdUp();
    }
}

void JustWrite::syncToLocalStorage() {
    QJsonObject local_storage;
    QJsonArray  book_data;

    //! FIXME: combine with local data

    for (const auto &[uuid, bm] : books_.asKeyValueRange()) {
        const auto &book_info = bm->info_ref();
        Q_ASSERT(uuid == book_info.uuid);
        QJsonObject book;
        book["book_id"]          = book_info.uuid;
        book["name"]             = book_info.title;
        book["author"]           = book_info.author;
        book["cover_url"]        = book_info.cover_url;
        book["creation_time"]    = book_info.creation_time.toString(Qt::ISODate);
        book["last_update_time"] = book_info.last_update_time.toString(Qt::ISODate);
        book_data.append(book);
    }

    local_storage["major_author"] = likely_author_;
    local_storage["data"]         = book_data;

    QDir dir{AppConfig::get_instance().path(AppConfig::StandardPath::UserData)};
    Q_ASSERT(dir.exists());

    //! TODO: check validity of local storage file

    QFile data_file(dir.filePath("mainfest.json"));
    data_file.open(QIODevice::WriteOnly | QIODevice::Text);
    data_file.write(QJsonDocument(local_storage).toJson());
    data_file.close();

    //! NOTE: here we simply sync to local according to the book set in the memory, and remove
    //! the book from the set also means remove the book from the local storage, however, in the
    //! current edition, we simply delete the record without removing the content from your
    //! machine, so you can mannually recover it by adding the record to the mainfest.json file
    //! FIXME: you know what I'm gonna say - yeah, that's not a good idea

    for (const auto &[uuid, bm] : books_.asKeyValueRange()) {
        if (!dir.exists(uuid)) { dir.mkdir(uuid); }
        dir.cd(uuid);

        QJsonArray volumes;
        for (const int vid : bm->get_volumes()) {
            QJsonObject volume;
            QJsonArray  chapters;
            for (const int cid : bm->get_chapters_of_volume(vid)) {
                QJsonObject chapter;
                chapter["cid"]   = cid;
                chapter["title"] = bm->get_title(cid).value().get();
                chapters.append(chapter);
            }
            volume["vid"]      = vid;
            volume["title"]    = bm->get_title(vid).value().get();
            volume["chapters"] = chapters;
            volumes.append(volume);
        }

        QFile toc_file(dir.filePath("TOC"));
        toc_file.open(QIODevice::WriteOnly | QIODevice::Text);
        toc_file.write(QJsonDocument(volumes).toJson());
        toc_file.close();

        //! FIXME: unsafe cast
        const auto book_manager = static_cast<BookManager *>(bm);
        for (const int cid : book_manager->get_all_chapters()) {
            if (!book_manager->is_chapter_dirty(cid)) { continue; }
            Q_ASSERT(book_manager->chapter_cached(cid));
            const auto content = std::move(book_manager->fetch_chapter_content(cid).value());
            QFile      file(book_manager->get_path_to_chapter(cid));
            file.open(QIODevice::WriteOnly | QIODevice::Text);
            Q_ASSERT(file.isOpen());
            file.write(content.toUtf8());
            file.close();
        }

        dir.cdUp();
    }
}

void JustWrite::switchToPage(PageType page) {
    Q_ASSERT(page_map_.contains(page));
    Q_ASSERT(page_map_.value(page, nullptr));
    if (auto w = page_map_[page]; w != ui_page_stack_->currentWidget()) {
        ui_page_stack_->setCurrentWidget(w);
    }
    current_page_ = page;
    emit pageChanged(page);
}

void JustWrite::closePage() {
    if (current_page_ == PageType::Edit) { ui_edit_page_->syncAndClearEditor(); }
    //! TODO: throw a dialog to confirm quiting the jwrite
    //! TODO: sync book content to local storage
    requestQuitApp();
}

void JustWrite::showEvent(QShowEvent *event) {
    ui_tray_icon_->hide();
}

void JustWrite::hideEvent(QHideEvent *event) {
    ui_tray_icon_->show();
}

} // namespace jwrite::ui
