#include <jwrite/ui/JustWrite.h>
#include <jwrite/ui/ScrollArea.h>
#include <jwrite/ui/BookInfoEdit.h>
#include <jwrite/ui/QuickTextInput.h>
#include <jwrite/ui/ColorThemeDialog.h>
#include <jwrite/ui/MessageBox.h>
#include <jwrite/ColorTheme.h>
#include <jwrite/ProfileUtils.h>
#include <qt-material/qtmaterialcircularprogress.h>
#include <QScrollBar>
#include <QVBoxLayout>
#include <QKeyEvent>
#include <QDateTime>
#include <QPainter>
#include <QFileDialog>
#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

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
        QDir dir{QCoreApplication::applicationDirPath()};
        dir.cd("data");
        dir.cd(AbstractBookManager::info_ref().uuid);
        return dir.filePath(QString::number(cid));
    }

private:
    QMap<int, QString> chapters_;
    QSet<int>          modified_;
};

JustWrite::JustWrite() {
    setupUi();
    setupConnections();

    page_map_[PageType::Gallery]->installEventFilter(this);
    page_map_[PageType::Edit]->installEventFilter(this);

    switchToPage(PageType::Gallery);
    setColorSchema(ColorSchema::Dark);

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
    for (auto book : books_) { delete book; }
    books_.clear();

    jwrite_profiler_dump(QString("jwrite-profiler.%1.log")
                             .arg(QDateTime::currentDateTime().toString("yyyyMMddHHmmss")));
}

void JustWrite::updateBookInfo(int index, const BookInfo &info) {
    auto book_info = info;

    if (book_info.author.isEmpty()) {
        book_info.author = getLikelyAuthor();
    } else {
        updateLikelyAuthor(book_info.author);
    }
    if (book_info.title.isEmpty()) { book_info.title = QString("未命名书籍-%1").arg(index + 1); }

    ui_gallery_->updateDisplayCaseItem(index, book_info);

    //! FIXME: only to ensure the sync-to-local-storage is available to access all the book
    //! items, remeber to remove this later as long as realtime sync has been done
    if (const auto &uuid = book_info.uuid; !books_.contains(uuid)) {
        auto bm        = new BookManager;
        bm->info_ref() = book_info;
        books_.insert(uuid, bm);
    } else {
        books_.value(uuid)->info_ref() = book_info;
    }
}

QString JustWrite::getLikelyAuthor() const {
    return likely_author_.isEmpty() ? "佚名" : likely_author_;
}

void JustWrite::updateLikelyAuthor(const QString &author) {
    //! TODO: save likely author to local storage
    if (!author.isEmpty() && likely_author_.isEmpty() && author != getLikelyAuthor()) {
        likely_author_ = author;
    }
}

void JustWrite::setColorSchema(ColorSchema schema) {
    ColorTheme theme = ColorTheme::from_schema(schema);
    updateColorTheme(theme);
}

void JustWrite::updateColorTheme(const ColorTheme &theme) {
    color_theme_ = theme;

    auto pal = palette();
    pal.setColor(QPalette::Window, color_theme_.Window);
    pal.setColor(QPalette::WindowText, color_theme_.WindowText);
    pal.setColor(QPalette::Base, color_theme_.TextBase);
    pal.setColor(QPalette::Text, color_theme_.Text);
    pal.setColor(QPalette::Highlight, color_theme_.SelectedText);
    pal.setColor(QPalette::Button, color_theme_.Window);
    pal.setColor(QPalette::ButtonText, color_theme_.WindowText);
    setPalette(pal);

    if (auto w = static_cast<QScrollArea *>(page_map_[PageType::Gallery])->verticalScrollBar()) {
        auto pal = w->palette();
        pal.setColor(w->backgroundRole(), color_theme_.TextBase);
        pal.setColor(w->foregroundRole(), color_theme_.Window);
        w->setPalette(pal);
    }

    ui_gallery_->updateColorTheme(color_theme_);
    ui_edit_page_->updateColorTheme(color_theme_);
}

void JustWrite::toggleMaximize() {
    if (isMinimized()) { return; }
    if (isMaximized()) {
        showNormal();
    } else {
        showMaximized();
    }
}

void JustWrite::setupUi() {
    auto top_layout     = new QVBoxLayout(this);
    auto container      = new QWidget;
    ui_top_most_layout_ = new QStackedLayout(container);

    ui_title_bar_ = new TitleBar;

    top_layout->addWidget(ui_title_bar_);
    top_layout->addWidget(container);

    ui_page_stack_  = new QStackedWidget;
    ui_gallery_     = new Gallery;
    ui_edit_page_   = new EditPage;
    ui_popup_layer_ = new ShadowOverlay;

    auto gallery_page = new ScrollArea;
    gallery_page->setWidget(ui_gallery_);

    ui_page_stack_->addWidget(gallery_page);
    ui_page_stack_->addWidget(ui_edit_page_);

    ui_top_most_layout_->addWidget(ui_page_stack_);
    ui_top_most_layout_->addWidget(ui_popup_layer_);

    ui_agent_ = new QWK::WidgetWindowAgent(this);
    ui_agent_->setup(this);
    ui_agent_->setTitleBar(ui_title_bar_);
    ui_agent_->setSystemButton(
        QWK::WidgetWindowAgent::Minimize, ui_title_bar_->systemButton(SystemButton::Minimize));
    ui_agent_->setSystemButton(
        QWK::WidgetWindowAgent::Maximize, ui_title_bar_->systemButton(SystemButton::Maximize));
    ui_agent_->setSystemButton(
        QWK::WidgetWindowAgent::Close, ui_title_bar_->systemButton(SystemButton::Close));

    top_layout->setContentsMargins({});
    top_layout->setSpacing(0);

    container->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

    ui_top_most_layout_->setStackingMode(QStackedLayout::StackAll);
    ui_top_most_layout_->setCurrentWidget(ui_popup_layer_);

    page_map_[PageType::Gallery] = gallery_page;
    page_map_[PageType::Edit]    = ui_edit_page_;
}

void JustWrite::setupConnections() {
    connect(ui_gallery_, &Gallery::clicked, this, [this](int index) {
        if (index == ui_gallery_->totalItems()) { requestUpdateBookInfo(index); }
    });
    connect(ui_gallery_, &Gallery::menuClicked, this, &JustWrite::requestBookAction);
    connect(this, &JustWrite::pageChanged, this, [this](PageType page) {
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
    });
    connect(ui_title_bar_, &TitleBar::minimizeRequested, this, &QWidget::showMinimized);
    connect(ui_title_bar_, &TitleBar::maximizeRequested, this, &JustWrite::toggleMaximize);
    connect(ui_title_bar_, &TitleBar::closeRequested, this, &JustWrite::closePage);
    connect(
        ui_edit_page_, &EditPage::renameTocItemRequested, this, &JustWrite::requestRenameTocItem);
}

void JustWrite::requestUpdateBookInfo(int index) {
    Q_ASSERT(index >= 0 && index <= ui_gallery_->totalItems());
    const bool on_insert = index == ui_gallery_->totalItems();

    auto info = on_insert ? BookInfo{.uuid = AbstractBookManager::alloc_uuid()}
                          : ui_gallery_->bookInfoAt(index);
    if (info.author.isEmpty()) { info.author = getLikelyAuthor(); }
    if (info.title.isEmpty()) { info.title = QString("未命名书籍-%1").arg(index + 1); }

    auto edit = new BookInfoEdit;
    edit->setBookInfo(info);

    showOverlay(edit);

    connect(edit, &BookInfoEdit::submitRequested, this, [this, index](BookInfo info) {
        updateBookInfo(index, info);
        closeOverlay();
    });
    connect(edit, &BookInfoEdit::cancelRequested, this, &JustWrite::closeOverlay);
    connect(edit, &BookInfoEdit::changeCoverRequested, this, [this, edit] {
        QImage     image;
        const auto path = requestImagePath(true, &image);
        if (path.isEmpty()) { return; }
        edit->setCover(path);
    });
}

QString JustWrite::requestImagePath(bool validate, QImage *out_image) {
    const auto filter = "图片 (*.bmp *.jpg *.jpeg *.png)";
    auto       path   = QFileDialog::getOpenFileName(this, "选择封面", "", filter);

    if (path.isEmpty()) { return ""; }

    if (!validate) { return path; }

    QImage image(path);
    if (image.isNull()) { return ""; }

    if (out_image) { *out_image = std::move(image); }

    return path;
}

void JustWrite::requestBookAction(int index, Gallery::MenuAction action) {
    Q_ASSERT(index >= 0 && index < ui_gallery_->totalItems());
    switch (action) {
        case Gallery::Open: {
            requestStartEditBook(index);
        } break;
        case Gallery::Edit: {
            requestUpdateBookInfo(index);
        } break;
        case Gallery::Delete: {
            auto dialog = new MessageBox;
            dialog->setCaption("删除书籍");
            dialog->setText("删除后，作品将无法恢复，请谨慎操作。");

            showOverlay(dialog);
            const int result = dialog->exec();
            closeOverlay();

            if (result == MessageBox::Yes) {
                const auto uuid = ui_gallery_->bookInfoAt(index).uuid;
                ui_gallery_->removeDisplayCase(index);
                //! NOTE: remove the book-manager means remove the book from the local storage
                //! when the jwrite exits, see syncToLocalStorage()
                //! FIXME: that's not a good idea
                auto bm = books_.value(uuid);
                books_.remove(uuid);
                delete bm;
            }
        } break;
    }
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

    ui_edit_page_->resetBookSource(bm);

    switchToPage(PageType::Edit);

    ui_edit_page_->focusOnEditor();

    if (const auto &chapters = bm->get_all_chapters(); !chapters.isEmpty()) {
        ui_edit_page_->openChapter(chapters.back());
    }
}

void JustWrite::requestRenameTocItem(const BookInfo &book_info, int vid, int cid) {
    auto input = new QuickTextInput;

    auto bm = books_.value(book_info.uuid);
    Q_ASSERT(bm);

    const int toc_id = cid == -1 ? vid : cid;
    Q_ASSERT(bm->has_toc_item(toc_id));

    if (cid == -1) {
        input->setPlaceholderText("请输入新分卷名");
        input->setLabel("分卷名");
    } else {
        input->setPlaceholderText("请输入新章节名");
        input->setLabel("章节名");
    }
    input->setText(bm->get_title(toc_id).value());

    showOverlay(input);

    connect(
        input,
        &QuickTextInput::submitRequested,
        this,
        [this, toc_id](QString text) {
            closeOverlay();
            ui_edit_page_->renameBookDirItem(toc_id, text);
            ui_edit_page_->focusOnEditor();
        },
        Qt::QueuedConnection);
    connect(
        input,
        &QuickTextInput::cancelRequested,
        this,
        [this] {
            closeOverlay();
            ui_edit_page_->focusOnEditor();
        },
        Qt::QueuedConnection);
}

void JustWrite::requestInitFromLocalStorage() {
    showProgressOverlay();

    const QDir home_dir{QCoreApplication::applicationDirPath()};
    if (home_dir.exists("data")) {
        loadDataFromLocalStorage();
    } else {
        initLocalStorage();
    }

    closeOverlay();
}

void JustWrite::requestQuitApp() {
    showProgressOverlay();
    //! TODO: wait other background jobs to be finished
    syncToLocalStorage();
    close();
}

void JustWrite::initLocalStorage() {
    QDir dir{QCoreApplication::applicationDirPath()};

    if (!dir.exists("data")) {
        const bool succeed = dir.mkdir("data");
        Q_ASSERT(succeed);
    }

    dir.cd("data");

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
    QDir dir{QCoreApplication::applicationDirPath()};

    Q_ASSERT(dir.exists("data"));
    dir.cd("data");

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
            .uuid      = uuid,
            .title     = name,
            .author    = book["author"].toString(""),
            .cover_url = book["cover_url"].toString(""),
        };

        updateBookInfo(ui_gallery_->totalItems(), book_info);

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
        book["book_id"]   = book_info.uuid;
        book["name"]      = book_info.title;
        book["author"]    = book_info.author;
        book["cover_url"] = book_info.cover_url;
        //! FIXME: record real update time
        book["last_update_time"] = QDateTime::currentDateTime().toString("yyyy-MM-dd.HH:mm:ss");
        //! TODO: export toc and contents
        book_data.append(book);
    }

    local_storage["major_author"]     = likely_author_;
    local_storage["last_update_time"] = QDateTime::currentDateTimeUtc().toString();
    local_storage["data"]             = book_data;

    QDir dir{QCoreApplication::applicationDirPath()};

    Q_ASSERT(dir.exists("data"));
    dir.cd("data");

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

void JustWrite::showOverlay(QWidget *widget) {
    ui_popup_layer_->setWidget(widget);
    ui_popup_layer_->show();
    widget->setFocus();
}

void JustWrite::closeOverlay() {
    ui_popup_layer_->hide();
    if (auto w = ui_popup_layer_->take()) { delete w; }
}

void JustWrite::showProgressOverlay() {
    showOverlay(new QtMaterialCircularProgress);
}

void JustWrite::switchToPage(PageType page) {
    Q_ASSERT(page_map_.contains(page));
    Q_ASSERT(page_map_.value(page, nullptr));
    if (auto w = page_map_[page]; w != ui_page_stack_->currentWidget()) {
        closeOverlay();
        ui_page_stack_->setCurrentWidget(w);
    }
    current_page_ = page;
    emit pageChanged(page);
}

void JustWrite::closePage() {
    switch (current_page_) {
        case PageType::Edit: {
            ui_edit_page_->syncAndClearEditor();
            switchToPage(PageType::Gallery);
        } break;
        case PageType::Gallery: {
            //! TODO: throw a dialog to confirm quiting the jwrite
            //! TODO: sync book content to local storage
            requestQuitApp();
        } break;
    }
}

bool JustWrite::eventFilter(QObject *watched, QEvent *event) {
    if (event->type() == QEvent::KeyPress) {
        auto e = static_cast<QKeyEvent *>(event);
        if (auto opt = command_manager_.match(e)) {
            if (*opt == GlobalCommand::ShowColorThemeDialog) {
                ColorThemeDialog dialog(getColorTheme(), this);
                connect(&dialog, &ColorThemeDialog::themeApplied, this, [this, &dialog] {
                    updateColorTheme(dialog.getTheme());
                });
                dialog.exec();
                disconnect(&dialog, &ColorThemeDialog::themeApplied, this, nullptr);
                updateColorTheme(dialog.getTheme());
                return true;
            }
        }
    }
    return false;
}

} // namespace jwrite::ui
