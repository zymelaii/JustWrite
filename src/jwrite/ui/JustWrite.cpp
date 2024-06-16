#include <jwrite/ui/JustWrite.h>
#include <jwrite/ui/ScrollArea.h>
#include <jwrite/ui/BookInfoEdit.h>
#include <jwrite/ui/ColorSchemeDialog.h>
#include <jwrite/ui/MessageBox.h>
#include <jwrite/ui/SettingsDialog.h>
#include <jwrite/ui/AboutDialog.h>
#include <jwrite/ui/ToolbarIcon.h>
#include <jwrite/ui/FontSelectDialog.h>
#include <jwrite/ColorScheme.h>
#include <jwrite/epub/EpubBuilder.h>
#include <jwrite/AppAction.h>
#include <jwrite/WordCounter.h>
#include <jwrite/ProfileUtils.h>
#include <widget-kit/TextInputDialog.h>
#include <widget-kit/OverlaySurface.h>
#include <spdlog/spdlog.h>
#include <QMouseEvent>
#include <QScrollBar>
#include <QVBoxLayout>
#include <QToolTip>
#include <QKeyEvent>
#include <QDateTime>
#include <QPainter>
#include <QFileDialog>
#include <QCoreApplication>
#include <QFontDatabase>
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
        spdlog::info("from book {}: sync chapter {}", info().uuid.toStdString(), cid);
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

bool JustWrite::do_load_book(const BookInfo &book_info) {
    if (books_.contains(book_info.uuid)) { return false; }
    auto bm        = new BookManager;
    bm->info_ref() = book_info;
    books_.insert(book_info.uuid, bm);
    return true;
}

void JustWrite::request_create_new_book() {
    BookInfo book_info{.uuid = BookManager::alloc_uuid()};
    Q_ASSERT(!books_.contains(book_info.uuid));
    do_create_book(book_info);
    ui_gallery_->updateDisplayCaseItem(ui_gallery_->totalItems(), book_info);
    request_update_book_info(book_info.uuid);
}

void JustWrite::do_create_book(BookInfo &book_info) {
    Q_ASSERT(!books_.contains(book_info.uuid));

    book_info.title            = tr("JustWrite.default_book_title");
    book_info.author           = get_default_author();
    book_info.cover_url        = ":/res/default-cover.png";
    book_info.creation_time    = QDateTime::currentDateTimeUtc();
    book_info.last_update_time = QDateTime::currentDateTimeUtc();

    do_load_book(book_info);
}

void JustWrite::request_remove_book(const QString &book_id) {
    Q_ASSERT(books_.contains(book_id));
    const auto choice = MessageBox::show(
        ui_surface_,
        tr("JustWrite.request_remove_book.caption"),
        tr("JustWrite.request_remove_book.text"),
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

void JustWrite::request_open_book(const QString &book_id) {
    Q_ASSERT(books_.contains(book_id));
    auto bm = books_.value(book_id);
    Q_ASSERT(bm);

    get_wait_builder()
        .withPolicy(Progress::UseMinimumDisplayTime, false)
        .withBlockingJob([this, bm] {
            ui_edit_page_->reset_source(bm);
            request_switch_page(AppConfig::Page::Edit);
            ui_edit_page_->request_invalidate_wcstate();
        })
        .withAsyncJob([this, bm] {
            ui_edit_page_->do_flush_wcstate();
            if (const auto &chapters = bm->get_all_chapters(); !chapters.isEmpty()) {
                ui_edit_page_->do_open_chapter(chapters.back());
            }
            ui_edit_page_->editor()->prepare_render_data();
        })
        .exec(ui_surface_);

    ui_edit_page_->focus_editor();
    ui_edit_page_->request_sync_wcstate();

    auto &config = AppConfig::get_instance();
    config.set_value(AppConfig::ValOption::LastEditingBookOnQuit, book_id);
}

void JustWrite::do_open_book(const QString &book_id) {
    //! TODO: preload chapters
}

void JustWrite::request_close_opened_book() {}

void JustWrite::do_close_book(const QString &book_id) {}

void JustWrite::request_update_book_info(const QString &book_id) {
    Q_ASSERT(books_.contains(book_id));
    const auto &initial = books_.value(book_id)->info_ref();
    if (auto opt = BookInfoEdit::get_book_info(ui_surface_, initial)) {
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
    if (book_info.title.isEmpty()) { book_info.title = tr("JustWrite.default_book_title"); }
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

    const auto caption = type == TocType::Volume
                           ? tr("JustWrite.request_rename_toc_item.volume_caption")
                           : tr("JustWrite.request_rename_toc_item.chapter_caption");
    const auto hint    = tr("JustWrite.request_rename_toc_item.placeholder");
    if (auto opt = TextInputDialog::getInputText(
            ui_surface_, bm->get_title(toc_id).value(), caption, hint)) {
        const auto &title = *opt;
        do_rename_toc_item(book_id, toc_id, title);
        //! FIXME: just notify the book dir to update
        ui_edit_page_->rename_toc_item(toc_id, title);
    }
}

void JustWrite::do_rename_toc_item(const QString &book_id, int toc_id, const QString &title) {
    auto bm = books_.value(book_id);
    Q_ASSERT(bm->has_toc_item(toc_id));
    if (bm->get_title(toc_id) == title) { return; }
    spdlog::info(
        "from book {}: rename {}[id={}]: \"{}\" -> \"{}\"",
        book_id.toStdString(),
        bm->has_volume(toc_id) ? "volume" : "chapter",
        toc_id,
        bm->get_title(toc_id).value().get().toLocal8Bit().toStdString(),
        title.toLocal8Bit().toStdString());
    const bool succeed = bm->update_title(toc_id, title);
    Q_ASSERT(succeed);
}

void JustWrite::request_export_book(const QString &book_id) {
    Q_ASSERT(books_.contains(book_id));
    auto bm = books_.value(book_id);

    if (const int cid = ui_edit_page_->book_dir()->selectedSubItem(); bm->has_chapter(cid)) {
        //! TODO: check if the chapter is dirty
        MessageBox::Option option{};
        option.type        = MessageBox::Type::Ternary;
        option.icon        = MessageBox::StandardIcon::Info;
        option.cancel_text = tr("JustWrite.request_export_book.ask_sync.cancel");
        option.yes_text    = tr("JustWrite.request_export_book.ask_sync.yes");
        option.no_text     = tr("JustWrite.request_export_book.ask_sync.no");
        const auto choice  = MessageBox::show(
            ui_surface_,
            tr("JustWrite.request_export_book.ask_sync.caption"),
            tr("JustWrite.request_export_book.ask_sync.text"),
            option);
        if (choice == MessageBox::Cancel) { return; }
        if (choice == MessageBox::Yes) {
            wait([&] {
                bm->sync_chapter_content(cid, ui_edit_page_->editor()->text());
            });
        }
    }

    const auto &book_info = bm->info_ref();
    const auto &title     = book_info.title;
    Q_ASSERT(!title.isEmpty());

    struct ExportInfo {
        ExportType type;
        QString    extesion;
    };

    QMap<QString, ExportInfo> filters{
        {tr("JustWrite.request_export_book.filter.plain_text"), {ExportType::PlainText, ".txt"}},
        {tr("JustWrite.request_export_book.filter.epub"),       {ExportType::ePub, ".epub"}    },
    };

    QString selected{};
    auto    path = QFileDialog::getSaveFileName(
        this,
        tr("JustWrite.request_export_book.caption"),
        title,
        filters.keys().join("\n"),
        &selected);

    if (path.isEmpty()) {
        MessageBox::Option option{};
        option.type = MessageBox::Type::Notify;
        option.icon = MessageBox::StandardIcon::Warning;
        MessageBox::show(
            ui_surface_,
            tr("JustWrite.request_export_book.throw_invalid_path.caption"),
            tr("JustWrite.request_export_book.throw_invalid_path.text"),
            option);
        return;
    }

    const auto &export_info = filters.value(selected);
    if (QFileInfo(path).suffix().isEmpty()) { path.append(export_info.extesion); }

    spdlog::info(
        "export book {} to {} with type {}",
        book_id.toStdString(),
        path.toLocal8Bit().toStdString(),
        magic_enum::enum_name(export_info.type));

    bool succeed = true;
    wait([&] {
        succeed = do_export_book(book_id, path, export_info.type);
    });

    if (succeed) {
        MessageBox::Option option{};
        option.type = MessageBox::Type::Notify;
        option.icon = MessageBox::StandardIcon::Info;
        MessageBox::show(
            ui_surface_,
            tr("JustWrite.request_export_book.success.caption"),
            tr("JustWrite.request_export_book.success.text"),
            option);
    } else {
        MessageBox::Option option{};
        option.type = MessageBox::Type::Notify;
        option.icon = MessageBox::StandardIcon::Error;
        MessageBox::show(
            ui_surface_,
            tr("JustWrite.request_export_book.unknown_error.caption"),
            tr("JustWrite.request_export_book.unknown_error.text"),
            option);
        spdlog::error("export book {} failed: unknown error", book_id.toStdString());
    }
}

bool JustWrite::do_export_book(const QString &book_id, const QString &path, ExportType type) {
    switch (type) {
        case ExportType::PlainText: {
            return do_export_book_as_plain_text(book_id, path);
        } break;
        case ExportType::ePub: {
            return do_export_book_as_epub(book_id, path);
        } break;
    }
}

bool JustWrite::do_export_book_as_plain_text(const QString &book_id, const QString &path) {
    Q_ASSERT(books_.contains(book_id));
    auto bm = books_.value(book_id);

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) { return false; }

    QTextStream out(&file);

    //! TODO: use specified toc format
    const int total_volumes = bm->get_volumes().size();
    int       chap_index    = 0;
    for (int i = 0; i < total_volumes; ++i) {
        const auto vid = bm->get_volumes()[i];
        out << tr("JustWrite.do_export_book_as_plain_text.default_volume_format")
                   .arg(i + 1)
                   .arg(bm->get_title(vid).value())
            << "\n\n";
        const auto &chaps = bm->get_chapters_of_volume(vid);
        for (const auto cid : chaps) {
            const auto content = bm->fetch_chapter_content(cid).value();
            out << tr("JustWrite.do_export_book_as_plain_text.default_chapter_format")
                       .arg(++chap_index)
                       .arg(bm->get_title(cid).value())
                << "\n"
                << content << "\n\n";
        }
    }

    return true;
}

bool JustWrite::do_export_book_as_epub(const QString &book_id, const QString &path) {
    Q_ASSERT(books_.contains(book_id));
    auto bm = books_.value(book_id);

    const auto &title  = bm->info_ref().title;
    const auto &author = bm->info_ref().author;
    Q_ASSERT(!title.isEmpty());
    Q_ASSERT(!author.isEmpty());

    epub::EpubBuilder builder(path);

    const int total_volumes = bm->get_volumes().size();
    for (int i = 0; i < total_volumes; ++i) {
        const int vid = bm->get_volumes()[i];
        builder.with_volume(
            tr("JustWrite.do_export_book_as_epub.default_volume_format")
                .arg(i + 1)
                .arg(bm->get_title(vid).value()),
            bm->get_chapters_of_volume(vid).size());
    }

    int global_chap_index = 0;
    builder.with_name(title)
        .with_author(author)
        .feed([this, bm, &global_chap_index](
                  int vol_index, int chap_index, QString &out_chap_title, QString &out_content) {
            const int vid  = bm->get_volumes()[vol_index];
            const int cid  = bm->get_chapters_of_volume(vid)[chap_index];
            out_chap_title = tr("JustWrite.do_export_book_as_epub.default_chapter_format")
                                 .arg(++global_chap_index)
                                 .arg(bm->get_title(cid).value());
            out_content = bm->fetch_chapter_content(cid).value();
        })
        .build();

    return true;
}

void JustWrite::request_init_from_local_storage() {
    const auto &config = AppConfig::get_instance();
    if (const QDir data_dir{config.path(AppConfig::StandardPath::UserData)}; !data_dir.exists()) {
        do_init_local_storage();
        Q_ASSERT(data_dir.exists());
    } else {
        do_load_local_storage();
        for (auto &bm : books_) {
            ui_gallery_->updateDisplayCaseItem(ui_gallery_->totalItems(), bm->info_ref());
        }
    }
}

void JustWrite::do_init_local_storage() {
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

void JustWrite::do_load_local_storage() {
    spdlog::info("sync data from local storage BEGIN -->");

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

        {
            const bool succeed = do_load_book(book_info);
            Q_ASSERT(succeed);
        }

        if (!dir.exists(uuid)) { continue; }

        {
            const bool succeed = dir.cd(uuid);
            Q_ASSERT(succeed);
        }

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

    spdlog::info("<-- DONE");
}

void JustWrite::do_sync_local_storage() {
    spdlog::info("sync data to local storage BEGIN -->");

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

    spdlog::info("<-- DONE");
}

void JustWrite::request_switch_page(AppConfig::Page page) {
    Q_ASSERT(page_map_.contains(page));
    Q_ASSERT(page_map_.value(page, nullptr));
    if (auto w = page_map_[page]; w != ui_page_stack_->currentWidget()) {
        ui_page_stack_->setCurrentWidget(w);
    }
    ui_toolbar_->apply_mask(page_toolbar_mask_[page]);
    current_page_ = page;
    emit on_page_change(page);
}

void JustWrite::set_fullscreen_mode(bool enable) {
    const auto ws     = windowState();
    const auto new_ws = enable ? (ws | Qt::WindowFullScreen) : (ws & ~Qt::WindowFullScreen);

    const auto ti_removed  = enable ? TI_ExitFullscreen : TI_Fullscreen;
    const auto ti_inserted = enable ? TI_Fullscreen : TI_ExitFullscreen;

    for (auto &mask : page_toolbar_mask_) {
        mask.remove(ti_removed);
        mask.insert(ti_inserted);
    }

    ui_toolbar_->apply_mask(page_toolbar_mask_[current_page_]);

    if (enable) {
        ui_title_bar_->setVisible(false);
        if (auto_hide_toolbar_on_fullscreen_) { ui_toolbar_->setVisible(false); }
        ui_toolbar_->installEventFilter(this);
    } else {
        ui_title_bar_->setVisible(true);
        ui_toolbar_->setVisible(true);
        ui_toolbar_->removeEventFilter(this);
    }

    setWindowState(new_ws);
    fullscreen_ = enable;
}

void JustWrite::trigger_shortcut(GlobalCommand shortcut) {
    emit on_trigger_shortcut(shortcut);
}

QString JustWrite::get_default_author() const {
    return likely_author_.isEmpty() ? tr("JustWrite.default_author") : likely_author_;
}

void JustWrite::set_default_author(const QString &author, bool force) {
    if (likely_author_.isEmpty() || force) { likely_author_ = author; }
}

void JustWrite::update_color_scheme(const ColorScheme &scheme) {
    auto pal = palette();
    pal.setColor(QPalette::Window, scheme.window());
    pal.setColor(QPalette::WindowText, scheme.window_text());
    pal.setColor(QPalette::Base, scheme.text_base());
    pal.setColor(QPalette::Text, scheme.text());
    pal.setColor(QPalette::Highlight, scheme.selected_text());
    pal.setColor(QPalette::Button, scheme.window());
    pal.setColor(QPalette::ButtonText, scheme.window_text());
    setPalette(pal);

    if (auto w =
            static_cast<QScrollArea *>(page_map_[AppConfig::Page::Gallery])->verticalScrollBar()) {
        auto pal = w->palette();
        pal.setColor(w->backgroundRole(), scheme.text_base());
        pal.setColor(w->foregroundRole(), scheme.window());
        w->setPalette(pal);
    }

    ui_title_bar_->update_color_scheme(scheme);
    ui_toolbar_->update_color_scheme(scheme);
    ui_gallery_->update_color_scheme(scheme);
    ui_edit_page_->update_color_scheme(scheme);

    auto font = this->font();
    font.setPointSize(10);
    QToolTip::setFont(font);

    pal.setColor(QPalette::ToolTipBase, scheme.window());
    pal.setColor(QPalette::ToolTipText, scheme.window_text());
    QToolTip::setPalette(pal);
}

void JustWrite::handle_gallery_on_click(int index) {
    if (index == ui_gallery_->totalItems()) { request_create_new_book(); }
}

void JustWrite::handle_gallery_on_menu_action(int index, Gallery::MenuAction action) {
    Q_ASSERT(index >= 0 && index < ui_gallery_->totalItems());
    const auto &book_id = ui_gallery_->bookInfoAt(index).uuid;
    switch (action) {
        case Gallery::Open: {
            request_open_book(book_id);
        } break;
        case Gallery::Edit: {
            request_update_book_info(book_id);
        } break;
        case Gallery::Delete: {
            request_remove_book(book_id);
        } break;
    }
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

void JustWrite::handle_edit_page_on_export() {
    Q_ASSERT(current_page_ == AppConfig::Page::Edit);
    const auto &book_id = ui_edit_page_->get_book_id_of_source();
    request_export_book(book_id);
}

void JustWrite::handle_on_page_change(AppConfig::Page page) {
    switch (page) {
        case AppConfig::Page::Gallery: {
            ui_title_bar_->setTitle(tr("JustWrite.default_title"));
        } break;
        case AppConfig::Page::Edit: {
            const auto &book_id = ui_edit_page_->get_book_id_of_source();
            Q_ASSERT(books_.contains(book_id));
            const auto &info = books_.value(book_id)->info_ref();
            const auto  title =
                tr("JustWrite.on_edit_title_prompt").arg(info.title).arg(info.author);
            ui_title_bar_->setTitle(title);
        } break;
    }
}

void JustWrite::handle_on_open_help() {
    AboutDialog::show(ui_surface_);
}

void JustWrite::handle_on_open_settings() {
    auto settings = std::make_unique<SettingsDialog>();

    connect(settings.get(), &SettingsDialog::on_request_editor_font_select, this, [&]() {
        auto      &config   = AppConfig::get_instance();
        const auto families = config.value(AppConfig::ValOption::TextFont).split(",");
        if (auto opt = FontSelectDialog::get_font_families(ui_surface_, families)) {
            config.set_value(AppConfig::ValOption::TextFont, opt->join(','));
        }
    });
    connect(settings.get(), &SettingsDialog::on_request_color_scheme_editor, this, [this]() {
        //! FIXME: implement custom color scheme editor
        auto &config = AppConfig::get_instance();

        const auto old_theme  = config.theme();
        const auto old_scheme = config.scheme();

        auto dialog = std::make_unique<ColorSchemeDialog>(old_theme, old_scheme, this);

        connect(
            dialog.get(),
            &ColorSchemeDialog::on_request_apply,
            this,
            [this, &config](ColorTheme theme, const ColorScheme &scheme) {
                config.set_theme(theme);
                config.set_scheme(scheme);
            });

        const int result = dialog->exec();

        if (result != QDialog::Accepted) {
            config.set_theme(old_theme);
            config.set_scheme(old_scheme);
        } else {
            config.set_theme(dialog->theme());
            config.set_scheme(dialog->scheme());
        }
    });

    settings->exec(ui_surface_);
}

void JustWrite::handle_on_theme_change() {
    const auto &config = AppConfig::get_instance();
    handle_on_scheme_change(config.scheme());
}

void JustWrite::handle_on_scheme_change(const ColorScheme &scheme) {
    update_color_scheme(scheme);
}

void JustWrite::handle_on_request_minimize() {
    showMinimized();
}

void JustWrite::handle_on_request_toggle_maximize() {
    if (isMinimized()) { return; }
    if (isMaximized()) {
        showNormal();
    } else {
        showMaximized();
    }
}

void JustWrite::handle_on_request_close() {
    if (current_page_ == AppConfig::Page::Edit) {
        MessageBox::Option opt{};
        opt.type          = MessageBox::Type::Confirm;
        opt.yes_text      = tr("JustWrite.handle_on_request_close.yes");
        opt.no_text       = tr("JustWrite.handle_on_request_close.no");
        const auto choice = MessageBox::show(
            ui_surface_,
            tr("JustWrite.handle_on_request_close.caption"),
            tr("JustWrite.handle_on_request_close.text"),
            opt);
        if (choice != MessageBox::Yes) { return; }
    }

    close();
    spdlog::info("client is closed");

    if (!testAttribute(Qt::WA_QuitOnClose)) { qApp->quit(); }
}

void JustWrite::handle_on_about_to_quit() {
    spdlog::info("client is about to quit");
    if (current_page_ == AppConfig::Page::Edit) { ui_edit_page_->sync_chapter_from_editor(); }
    wait([this] {
        do_sync_local_storage();
    });
}

void JustWrite::handle_on_open_gallery() {
    if (current_page_ == AppConfig::Page::Gallery) { return; }
    if (current_page_ == AppConfig::Page::Edit) {
        wait([this] {
            ui_edit_page_->sync_chapter_from_editor();
        });
    }
    request_switch_page(AppConfig::Page::Gallery);
}

void JustWrite::handle_on_enter_fullscreen() {
    set_fullscreen_mode(true);
}

void JustWrite::handle_on_exit_fullscreen() {
    set_fullscreen_mode(false);
}

void JustWrite::handle_on_visiblity_change(bool visible) {
    ui_tray_icon_->setVisible(visible);
}

void JustWrite::handle_on_trigger_shortcut(GlobalCommand shortcut) {
    switch (shortcut) {
        case GlobalCommand::ToggleFullscreen: {
            set_fullscreen_mode(!fullscreen_);
        } break;
        case GlobalCommand::ToggleSidebar: {
            if (current_page_ == AppConfig::Page::Edit) { ui_edit_page_->toggle_sidebar(); }
        } break;
        case GlobalCommand::ToggleSoftCenterMode: {
            if (current_page_ == AppConfig::Page::Edit) {
                ui_edit_page_->toggle_soft_center_mode();
            }
        } break;
        case GlobalCommand::CreateNewChapter: {
            if (current_page_ == AppConfig::Page::Edit) {
                ui_edit_page_->handle_on_create_chapter();
            }
        } break;
        case GlobalCommand::Rename: {
            if (current_page_ == AppConfig::Page::Edit) {
                ui_edit_page_->handle_on_rename_selected_toc_item();
            }
        } break;
    }
}

void JustWrite::handle_config_on_option_change(AppConfig::Option opt, bool on) {
    using Option = AppConfig::Option;
    switch (opt) {
        case Option::AutoHideToolbarOnFullscreen: {
            auto_hide_toolbar_on_fullscreen_ = on;
            if (!auto_hide_toolbar_on_fullscreen_) {
                ui_toolbar_->setVisible(true);
            } else if (is_fullscreen_mode()) {
                ui_toolbar_->setVisible(false);
            }
        } break;
        case Option::FirstLineIndent: {
        } break;
        case Option::ElasticTextViewResize: {
            ui_edit_page_->editor()->set_elastic_resize_enabled(on);
        } break;
        case Option::AutoChapter: {
        } break;
        case Option::PairingSymbolMatch: {
        } break;
        case Option::CriticalChapterLimit: {
        } break;
        case Option::ChapterLimit: {
        } break;
        case Option::TimingBackup: {
        } break;
        case Option::QuantitativeBackup: {
        } break;
        case Option::BackupSmartMerge: {
        } break;
        case Option::KeyVersionRecognition: {
        } break;
        case Option::StrictWordCount: {
            if (on) {
                ui_edit_page_->reset_word_counter(new StrictWordCounter);
            } else {
                ui_edit_page_->reset_word_counter(new LossenWordCounter);
            }
        } break;
        case Option::SmoothScroll: {
            ui_edit_page_->editor()->set_smooth_scroll_enabled(on);
        } break;
    }
}

void JustWrite::handle_config_on_value_change(AppConfig::ValOption opt, const QString &value) {
    using Option = AppConfig::ValOption;
    auto &config = AppConfig::get_instance();
    switch (opt) {
        case Option::Language: {
            auto       translator = new QTranslator;
            const bool succeed    = translator->load(QString(":/lang.%1").arg(value));
            if (!succeed) {
                delete translator;
                return;
            }
            QApplication::installTranslator(translator);
        } break;
        case Option::PrimaryPage: {
        } break;
        case Option::DefaultEditMode: {
        } break;
        case Option::TextFont: {
            if (value.isEmpty()) {
                //! NOTE: point size here is not used but only for the fn call
                const auto default_font = config.font(AppConfig::FontStyle::Light, 16);
                ui_edit_page_->editor()->set_font(default_font);
            } else {
                QStringList families{};
                for (const auto &family : value.split(',')) {
                    if (QFontDatabase::hasFamily(family)) { families << family; }
                }
                if (families.empty()) { break; }
                ui_edit_page_->editor()->set_font(QFont(families));
            }
        } break;
        case Option::TextFontSize: {
            bool      ok   = false;
            const int size = value.toUInt(&ok);
            if (!ok) { break; }
            ui_edit_page_->editor()->set_font_size(size);
        } break;
        case Option::LineSpacingRatio: {
            bool         ok    = false;
            const double ratio = value.toDouble(&ok);
            if (!ok) { break; }
            ui_edit_page_->editor()->set_line_spacing_ratio(ratio);
        } break;
        case Option::BlockSpacing: {
            bool      ok      = false;
            const int spacing = value.toDouble(&ok);
            if (!ok) { break; }
            ui_edit_page_->editor()->set_block_spacing(spacing);
        } break;
        case Option::BookDirStyle: {
        } break;
        case Option::AutoChapterThreshold: {
        } break;
        case Option::CriticalChapterLimit: {
        } break;
        case Option::ChapterLimit: {
        } break;
        case Option::TimingBackupInterval: {
        } break;
        case Option::QuantitativeBackupThreshold: {
        } break;
        case Option::BackgroundImage: {
        } break;
        case Option::EditorBackgroundImage: {
        } break;
        case Option::BackgroundImageOpacity: {
        } break;
        case Option::ToolbarIconSize: {
            bool      ok   = false;
            const int size = value.toUInt(&ok);
            if (!ok) { break; }
            ui_toolbar_->set_icon_size(size);
        } break;
        case Option::LastEditingBookOnQuit: {
        } break;
        case Option::UnfocusedTextOpacity: {
            bool         ok      = false;
            const double opacity = value.toDouble(&ok);
            if (!ok) { break; }
            ui_edit_page_->editor()->set_unfocused_text_opacity(opacity);
        } break;
        case Option::TextFocusMode: {
            const auto focus_mode = AppConfig::get_text_focus_mode_name(value.toLower());
            ui_edit_page_->editor()->set_text_focus_mode(focus_mode);
        } break;
        case Option::CentreEditLine: {
            if (const auto mode = value.toLower(); mode == "never") {
                ui_edit_page_->editor()->set_auto_centre_mode(Editor::AutoCentre::Never);
            } else if (mode == "always") {
                ui_edit_page_->editor()->set_auto_centre_mode(Editor::AutoCentre::Always);
            } else if (mode == "adaptive") {
                ui_edit_page_->editor()->set_auto_centre_mode(Editor::AutoCentre::Adaptive);
            } else {
                Q_UNREACHABLE();
            }
        } break;
    }
}

JustWrite::JustWrite()
    : auto_hide_toolbar_on_fullscreen_{true} {
    setupUi();
    setupConnections();
    setMouseTracking(true);

    set_fullscreen_mode(windowState().testFlag(Qt::WindowFullScreen));

    init();
}

JustWrite::~JustWrite() {
    for (auto book : books_) { delete book; }
    books_.clear();

#if 0
    jwrite_profiler_dump(QString("jwrite-profiler.%1.log")
                             .arg(QDateTime::currentDateTimeUtc().toString("yyyyMMddHHmmss")));
#endif
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

    struct ToolbarItem {
        ToolbarItemType   type;
        QString           tip;
        QString           icon;
        AppAction::Action action;
        bool              bottom_side;
    };

    QList<ToolbarItem> toolbar_items{
        {TI_Gallery,
         tr("JustWrite.toolbar_item.gallery"),
         "toolbar/gallery",                                                      AppAction::OpenBookGallery,
         false                                                                                                    },
        {TI_Draft,
         tr("JustWrite.toolbar_item.draft"),
         "toolbar/draft",                                                        AppAction::OpenEditor,
         false                                                                                                    },
        {TI_Favorites,
         tr("JustWrite.toolbar_item.favorites"),
         "toolbar/favorites",                                                    AppAction::OpenFavorites,
         false                                                                                                    },
        {TI_Trash,
         tr("JustWrite.toolbar_item.trash"),
         "toolbar/trash",                                                        AppAction::OpenTrashBin,
         false                                                                                                    },
        {TI_Export,
         tr("JustWrite.toolbar_item.export"),
         "toolbar/export",                                                       AppAction::Export,
         false                                                                                                    },
        {TI_Share,          tr("JustWrite.toolbar_item.share"), "toolbar/share", AppAction::Share,           false},
        {TI_Fullscreen,
         tr("JustWrite.toolbar_item.fullscreen"),
         "toolbar/fullscreen",                                                   AppAction::Fullscreen,
         true                                                                                                     },
        {TI_ExitFullscreen,
         tr("JustWrite.toolbar_item.exit_fullscreen"),
         "toolbar/fullscreen-exit",                                              AppAction::ExitFullscreen,
         true                                                                                                     },
        {TI_Help,           tr("JustWrite.toolbar_item.help"),  "toolbar/help",  AppAction::OpenHelp,        true },
        {TI_Settings,
         tr("JustWrite.toolbar_item.settings"),
         "toolbar/settings",                                                     AppAction::OpenSettings,
         true                                                                                                     },
    };

    const auto &actions = AppAction::get_instance();
    for (int index = 0; index < toolbar_items.size(); ++index) {
        const auto &[type, tip, icon, action, bottom_side] = toolbar_items[index];
        Q_ASSERT(type == index);
        ui_toolbar_->add_item(tip, icon, actions.get(action), bottom_side);
    }

    page_toolbar_mask_.insert(
        AppConfig::Page::Gallery, {TI_Gallery, TI_Export, TI_Share, TI_ExitFullscreen});
    page_toolbar_mask_.insert(AppConfig::Page::Edit, {TI_Draft, TI_ExitFullscreen});

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

    page_map_[AppConfig::Page::Gallery] = gallery_page;
    page_map_[AppConfig::Page::Edit]    = ui_edit_page_;

    ui_tray_icon_ = new QSystemTrayIcon(this);
    ui_tray_icon_->setIcon(QIcon(":/app.ico"));
    ui_tray_icon_->setToolTip(tr("JustWrite.tray_icon.tooltip"));
    ui_tray_icon_->setVisible(false);

    update_color_scheme(AppConfig::get_instance().scheme());
}

void JustWrite::setupConnections() {
    auto &config  = AppConfig::get_instance();
    auto &actions = AppAction::get_instance();

    actions.bind(AppAction::OpenBookGallery, this, &JustWrite::handle_on_open_gallery);
    actions.bind(AppAction::OpenHelp, this, &JustWrite::handle_on_open_help);
    actions.bind(AppAction::OpenSettings, this, &JustWrite::handle_on_open_settings);
    actions.bind(AppAction::Export, this, &JustWrite::handle_edit_page_on_export);
    actions.bind(AppAction::Fullscreen, this, &JustWrite::handle_on_enter_fullscreen);
    actions.bind(AppAction::ExitFullscreen, this, &JustWrite::handle_on_exit_fullscreen);

    actions.attach(AppAction::OpenBookGallery, ui_edit_page_, &EditPage::on_request_quit_edit);
    actions.attach(AppAction::OpenSettings, ui_edit_page_, &EditPage::on_request_open_settings);

    connect(&config, &AppConfig::on_theme_change, this, &JustWrite::handle_on_theme_change);
    connect(&config, &AppConfig::on_scheme_change, this, &JustWrite::handle_on_scheme_change);
    connect(
        &config, &AppConfig::on_option_change, this, &JustWrite::handle_config_on_option_change);
    connect(&config, &AppConfig::on_value_change, this, &JustWrite::handle_config_on_value_change);
    connect(
        ui_title_bar_, &TitleBar::minimizeRequested, this, &JustWrite::handle_on_request_minimize);
    connect(
        ui_title_bar_,
        &TitleBar::maximizeRequested,
        this,
        &JustWrite::handle_on_request_toggle_maximize);
    connect(ui_title_bar_, &TitleBar::closeRequested, this, &JustWrite::handle_on_request_close);
    connect(qApp, &QApplication::aboutToQuit, this, &JustWrite::handle_on_about_to_quit);
    connect(ui_gallery_, &Gallery::clicked, this, &JustWrite::handle_gallery_on_click);
    connect(ui_gallery_, &Gallery::menuClicked, this, &JustWrite::handle_gallery_on_menu_action);
    connect(
        ui_edit_page_,
        &EditPage::on_request_rename_toc_item,
        this,
        &JustWrite::handle_book_dir_on_rename_toc_item__adapter);
    connect(
        this,
        &JustWrite::on_page_change,
        this,
        &JustWrite::handle_on_page_change,
        Qt::QueuedConnection);
    connect(this, &JustWrite::on_trigger_shortcut, this, &JustWrite::handle_on_trigger_shortcut);
}

void JustWrite::init() {
    auto &config = AppConfig::get_instance();

    //! NOTE: Qt gives out an unexpected minimum height to my widgets and QLayout::invalidate()
    //! could not solve the problem, maybe there is some dirty cached data at the bottom level.
    //! I eventually found that an explicit call to the expcted-to-be-readonly method sizeHint()
    //! could make effects on the confusing issue. so, **DO NOT TOUCH THE CODE** unless you
    //! figure out a better way to solve the problem
    const auto DO_NOT_REMOVE_THIS_STATEMENT = sizeHint();

    request_init_from_local_storage();

    for (const auto &opt : magic_enum::enum_values<AppConfig::Option>()) {
        handle_config_on_option_change(opt, config.option_enabled(opt));
    }
    for (const auto &opt : magic_enum::enum_values<AppConfig::ValOption>()) {
        handle_config_on_value_change(opt, config.value(opt));
    }

    auto       page    = config.primary_page();
    const auto book_id = config.value(AppConfig::ValOption::LastEditingBookOnQuit);
    if (page == AppConfig::Page::Edit && !books_.contains(book_id)) {
        page = AppConfig::Page::Gallery;
    }
    if (page == AppConfig::Page::Edit) {
        request_open_book(book_id);
    } else {
        request_switch_page(page);
    }
}

bool JustWrite::eventFilter(QObject *watched, QEvent *event) {
    if (watched == ui_toolbar_ && event->type() == QEvent::Leave) {
        Q_ASSERT(fullscreen_);
        if (auto_hide_toolbar_on_fullscreen_) { ui_toolbar_->hide(); }
    }
    return QWidget::eventFilter(watched, event);
}

void JustWrite::mouseMoveEvent(QMouseEvent *event) {
    //! NOTE: depends on JwriteApplication to capture mouse-move-events from child widgets
    if (fullscreen_ && ui_toolbar_->isHidden()) {
        if (const auto pos = event->pos(); pos.x() >= 0 && pos.x() <= 8) { ui_toolbar_->show(); }
    }
}

void JustWrite::showEvent(QShowEvent *event) {
    handle_on_visiblity_change(false);
}

void JustWrite::hideEvent(QHideEvent *event) {
    handle_on_visiblity_change(true);
}

} // namespace jwrite::ui
