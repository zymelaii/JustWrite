#include <jwrite/ui/EditPage.h>
#include <jwrite/ui/ScrollArea.h>
#include <jwrite/TextInputCommand.h>
#include <jwrite/ProfileUtils.h>
#include <jwrite/epub/EpubBuilder.h>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QScrollBar>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QSplitter>
#include <QDateTime>
#include <QFrame>
#include <QPainter>
#include <QMenu>
#include <QFileDialog>
#include <QMessageBox>

namespace jwrite::ui {

using epub::EpubBuilder;

class BookModel : public TwoLevelDataModel {
public:
    explicit BookModel(AbstractBookManager *bm, QObject *parent)
        : TwoLevelDataModel(parent)
        , bm_{bm} {
        Q_ASSERT(bm_);
    }

    int total_top_items() const override {
        return bm_->get_volumes().size();
    }

    int total_sub_items() const override {
        return bm_->get_all_chapters().size();
    }

    int total_items_under_top_item(int top_item_id) const override {
        return bm_->get_chapters_of_volume(top_item_id).size();
    }

    int top_item_at(int index) override {
        return bm_->get_volumes()[index];
    }

    int sub_item_at(int top_item_id, int index) const override {
        return bm_->get_chapters_of_volume(top_item_id)[index];
    }

    QList<int> get_sub_items(int top_item_id) const override {
        return bm_->get_chapters_of_volume(top_item_id);
    }

    bool has_top_item(int id) const override {
        return bm_->has_volume(id);
    }

    bool has_sub_item(int id) const override {
        return bm_->has_chapter(id);
    }

    bool has_sub_item_strict(int top_item_id, int sub_item_id) const override {
        if (!has_top_item(top_item_id)) { return false; }
        return get_sub_items(top_item_id).contains(sub_item_id);
    }

    int add_top_item(int index, const QString &value) override {
        const int id = bm_->add_volume(index, value);
        emit valueChanged(id, value);
        return id;
    }

    int add_sub_item(int top_item_id, int index, const QString &value) override {
        const int id = bm_->add_chapter(top_item_id, index, value);
        emit valueChanged(id, value);
        return id;
    }

    QString value(int id) const override {
        if (auto opt = bm_->get_title(id)) {
            return opt.value();
        } else {
            return "";
        }
    }

    void set_value(int id, const QString &value) override {
        bm_->update_title(id, value);
        emit valueChanged(id, value);
    }

private:
    AbstractBookManager *const bm_;
};

class BookDirItemRenderProxy
    : public TwoLevelTree::ItemRenderProxy
    , public QObject {
public:
    explicit BookDirItemRenderProxy(QObject *parent)
        : QObject(parent) {}

    void render(QPainter *p, const QRect &clip_bb, const TwoLevelTree::ItemInfo &item_info) {
        const auto format =
            item_info.is_top_item ? QStringLiteral("第 %1 卷 %2") : QStringLiteral("第 %1 章 %2");
        const auto title = format.arg(item_info.level_index + 1).arg(item_info.value);
        const auto flags = Qt::AlignLeft | Qt::AlignVCenter;
        p->drawText(clip_bb, flags, title);
    }
};

EditPage::EditPage(QWidget *parent)
    : QWidget(parent)
    , current_cid_{-1}
    , chap_words_{0}
    , total_words_{0}
    , messy_input_{new MessyInputWorker(this)}
    , book_manager_{nullptr} {
    setupUi();
    setupConnections();

    command_manager_.load_default();

    ui_editor->installEventFilter(this);

    sec_timer_.setInterval(1000);
    sec_timer_.setSingleShot(false);
    sec_timer_.start();
}

EditPage::~EditPage() {
    jwrite_profiler_dump(QStringLiteral("jwrite-profiler.%1.log")
                             .arg(QDateTime::currentDateTime().toString("yyyyMMddHHmmss")));

    messy_input_->kill();
    delete messy_input_;
}

void EditPage::updateColorTheme(const ColorTheme &color_theme) {
    auto pal = palette();
    pal.setColor(QPalette::Window, color_theme.Window);
    pal.setColor(QPalette::WindowText, color_theme.WindowText);
    pal.setColor(QPalette::Base, color_theme.TextBase);
    pal.setColor(QPalette::Text, color_theme.Text);
    pal.setColor(QPalette::Highlight, color_theme.SelectedText);
    pal.setColor(QPalette::Button, color_theme.Window);
    pal.setColor(QPalette::ButtonText, color_theme.WindowText);
    setPalette(pal);

    if (auto w = ui_named_widgets.value("sidebar.splitter")) {
        auto pal = w->palette();
        pal.setColor(QPalette::Window, color_theme.TextBase);
        w->setPalette(pal);
    }

    if (auto w = ui_sidebar) {
        auto pal = w->palette();
        pal.setColor(QPalette::Window, color_theme.Panel);
        w->setPalette(pal);
    }

    if (auto w = ui_editor) {
        auto pal = w->palette();
        pal.setColor(QPalette::Window, color_theme.TextBase);
        pal.setColor(QPalette::HighlightedText, color_theme.SelectedItem);
        w->setPalette(pal);
    }

    if (auto w = ui_named_widgets.value("sidebar.button-line")) {
        auto pal = w->palette();
        pal.setColor(QPalette::Base, color_theme.Panel);
        pal.setColor(QPalette::Button, color_theme.Hover);
        w->setPalette(pal);
    }

    if (auto w = ui_named_widgets.value("sidebar.sep-line")) {
        auto pal = w->palette();
        pal.setColor(QPalette::WindowText, color_theme.Border);
        w->setPalette(pal);
    }

    if (auto w = static_cast<QScrollArea *>(ui_named_widgets.value("sidebar.book-dir-container"))
                     ->verticalScrollBar()) {
        auto pal = w->palette();
        pal.setColor(w->backgroundRole(), color_theme.Panel);
        pal.setColor(w->foregroundRole(), color_theme.Window);
        w->setPalette(pal);
    }

    if (auto w = ui_book_dir) {
        auto pal = w->palette();
        pal.setColor(QPalette::Highlight, color_theme.SelectedItem);
        pal.setColor(QPalette::HighlightedText, color_theme.Hover);
        w->setPalette(pal);
    }
}

AbstractBookManager *EditPage::resetBookSource(AbstractBookManager *book_manager) {
    Q_ASSERT(book_manager);

    if (book_manager == book_manager_) { return nullptr; }

    auto old_book_manager = takeBookSource();

    book_manager_ = book_manager;

    if (auto last_model = ui_book_dir->setModel(new BookModel(book_manager_, ui_book_dir))) {
        delete last_model;
    }

    flushWordsCount();

    return old_book_manager;
}

AbstractBookManager *EditPage::takeBookSource() {
    if (!book_manager_) { return nullptr; }

    auto old_book_manager = book_manager_;

    book_manager_ = nullptr;
    chapter_locs_.clear();
    current_cid_ = -1;

    total_words_ = 0;
    chap_words_  = 0;

    return old_book_manager;
}

int EditPage::addVolume(int index, const QString &title) {
    Q_ASSERT(book_manager_);
    Q_ASSERT(index >= 0 && index <= ui_book_dir->totalTopItems());
    return ui_book_dir->addTopItem(index, title);
}

int EditPage::addChapter(int volume_index, const QString &title) {
    Q_ASSERT(book_manager_);
    Q_ASSERT(volume_index >= 0 && volume_index < ui_book_dir->totalTopItems());
    const auto vid = ui_book_dir->topItemAt(volume_index);
    Q_ASSERT(vid != -1);
    const int chap_index = ui_book_dir->totalSubItemsUnderTopItem(vid);
    return ui_book_dir->addSubItem(vid, chap_index, title);
}

void EditPage::openChapter(int cid) {
    Q_ASSERT(book_manager_);

    if (messy_input_->is_running()) { return; }

    if (cid == current_cid_) { return; }

    jwrite_profiler_start(SwitchChapter);

    const int next_cid = cid;
    const int last_cid = current_cid_;

    auto text   = book_manager_->has_chapter(next_cid)
                    ? book_manager_->fetch_chapter_content(next_cid).value()
                    : QString{};
    chap_words_ = text.length();

    if (last_cid != -1) {
        const auto loc = ui_editor->currentTextLoc();
        if (loc.block_index != -1) { chapter_locs_[last_cid] = loc; }
    }

    ui_editor->reset(text, true);
    book_manager_->sync_chapter_content(last_cid, text);

    if (chapter_locs_.contains(next_cid)) {
        const auto loc = chapter_locs_[next_cid];
        ui_editor->setCursorToTextLoc(loc);
    }

    current_cid_ = next_cid;

    //! TODO: provide interface in AbstractBookManager to get volume id of a chapter
    for (const int vid : book_manager_->get_volumes()) {
        if (!book_manager_->get_chapters_of_volume(vid).contains(cid)) { continue; }
        ui_book_dir->setSubItemSelected(vid, cid);
        ui_book_dir->setTopItemEllapsed(vid, false);
    }

    jwrite_profiler_record(SwitchChapter);
}

void EditPage::syncAndClearEditor() {
    if (current_cid_ == -1) { return; }

    Q_ASSERT(book_manager_);

    book_manager_->sync_chapter_content(current_cid_, ui_editor->take());

    current_cid_ = -1;

    flushWordsCount();
}

void EditPage::renameBookDirItem(int id, const QString &title) {
    Q_ASSERT(book_manager_);
    ui_book_dir->setItemValue(id, title);
}

void EditPage::exportToLocal(const QString &path, ExportType type) {
    Q_ASSERT(book_manager_);

    switch (type) {
        case ExportType::PlainText: {
            QFile file(path);
            if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                //! TODO: replace with jwrite style message box
                QMessageBox::warning(this, "导出失败", "无法打开文件：" + path);
                return;
            }

            QTextStream out(&file);

            const int total_volumes = book_manager_->get_volumes().size();
            int       chap_index    = 0;
            for (int i = 0; i < total_volumes; ++i) {
                const auto vid = book_manager_->get_volumes()[i];
                out << QStringLiteral("【第 %1 卷 %2】\n\n")
                           .arg(i + 1)
                           .arg(book_manager_->get_title(vid).value());
                const auto &chaps = book_manager_->get_chapters_of_volume(vid);
                for (const auto cid : chaps) {
                    const auto content = current_cid_ == cid
                                           ? ui_editor->text()
                                           : book_manager_->fetch_chapter_content(cid).value();
                    out << QStringLiteral("第 %1 章 %2\n")
                               .arg(++chap_index)
                               .arg(book_manager_->get_title(cid).value())
                        << content << "\n\n";
                }
            }
        } break;
        case ExportType::ePub: {
            const auto &title  = book_manager_->info_ref().title;
            const auto &author = book_manager_->info_ref().author;

            EpubBuilder builder(path);
            const int   total_volumes = book_manager_->get_volumes().size();
            for (int i = 0; i < total_volumes; ++i) {
                const int vid = book_manager_->get_volumes()[i];
                builder.with_volume(
                    QString("第 %1 卷 %2").arg(i + 1).arg(book_manager_->get_title(vid).value()),
                    book_manager_->get_chapters_of_volume(vid).size());
            }
            int global_chap_index = 0;
            builder.with_name(title.isEmpty() ? "未命名书籍" : title)
                .with_author(author.isEmpty() ? "佚名" : author)
                .feed([this, &global_chap_index](
                          int      vol_index,
                          int      chap_index,
                          QString &out_chap_title,
                          QString &out_content) {
                    const int vid  = book_manager_->get_volumes()[vol_index];
                    const int cid  = book_manager_->get_chapters_of_volume(vid)[chap_index];
                    out_chap_title = QString("第 %1 章 %2")
                                         .arg(++global_chap_index)
                                         .arg(book_manager_->get_title(cid).value());
                    out_content = current_cid_ == cid
                                    ? ui_editor->text()
                                    : book_manager_->fetch_chapter_content(cid).value();
                })
                .build();
        } break;
    }
}

void EditPage::setupUi() {
    auto layout         = new QVBoxLayout(this);
    auto content        = new QWidget;
    auto layout_content = new QHBoxLayout(content);
    auto splitter       = new QSplitter;
    ui_named_widgets.insert("sidebar.splitter", splitter);

    ui_editor     = new Editor;
    ui_status_bar = new StatusBar;

    ui_sidebar         = new QWidget;
    ui_new_volume      = new FlatButton;
    ui_new_chapter     = new FlatButton;
    ui_export_to_local = new FlatButton;
    ui_book_dir        = new TwoLevelTree;

    auto btn_line        = new QWidget;
    auto btn_line_layout = new QHBoxLayout(btn_line);
    btn_line_layout->setContentsMargins({});
    btn_line_layout->addStretch();
    btn_line_layout->addWidget(ui_new_volume);
    btn_line_layout->addStretch();
    btn_line_layout->addWidget(ui_new_chapter);
    btn_line_layout->addStretch();
    btn_line_layout->addWidget(ui_export_to_local);
    btn_line_layout->addStretch();
    ui_named_widgets.insert("sidebar.button-line", btn_line);

    auto sep_line = new QFrame;
    sep_line->setFrameShape(QFrame::HLine);
    ui_named_widgets.insert("sidebar.sep-line", sep_line);

    auto book_dir_container = new ScrollArea;
    book_dir_container->setWidget(ui_book_dir);
    ui_named_widgets.insert("sidebar.book-dir-container", book_dir_container);

    auto sidebar_layout = new QVBoxLayout(ui_sidebar);
    sidebar_layout->setContentsMargins(0, 10, 0, 10);
    sidebar_layout->addWidget(btn_line);
    sidebar_layout->addWidget(sep_line);
    sidebar_layout->addWidget(book_dir_container);

    ui_sidebar->setAutoFillBackground(true);
    ui_sidebar->setMinimumWidth(200);

    splitter->addWidget(ui_sidebar);
    splitter->addWidget(ui_editor);

    layout->addWidget(content);
    layout->addWidget(ui_status_bar);
    layout->setContentsMargins({});
    layout->setSpacing(0);

    layout_content->addWidget(splitter);
    layout_content->setContentsMargins({});

    splitter->setOrientation(Qt::Horizontal);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setCollapsible(0, false);
    splitter->setCollapsible(1, false);

    ui_new_volume->setText("新建卷");
    ui_new_chapter->setText("新建章");
    ui_export_to_local->setText("导出");

    ui_total_words = ui_status_bar->addItem("字数统计中...", false);
    ui_datetime    = ui_status_bar->addItem("", true);

    auto font = this->font();
    font.setPointSize(10);
    ui_sidebar->setFont(font);

    content->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

    //! install book dir item render proxy
    ui_book_dir->setItemRenderProxy(new BookDirItemRenderProxy(this));

    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    setAutoFillBackground(true);
}

void EditPage::setupConnections() {
    connect(ui_editor, &Editor::activated, this, [this] {
        if (current_cid_ != -1) { return; }
        if (book_manager_->get_all_chapters().empty()) {
            createAndOpenNewChapter();
        } else {
            //! TODO: open the last opened chapter
            //! ATTENTION: here we assume that the returned chapters are in order
            const int cid = book_manager_->get_all_chapters().last();
            openChapter(cid);
        }
    });
    connect(ui_editor, &Editor::textChanged, this, &EditPage::updateWordsCount);
    connect(ui_book_dir, &TwoLevelTree::subItemSelected, this, [this](int vid, int cid) {
        openChapter(cid);
    });
    connect(ui_new_volume, &FlatButton::pressed, this, [this] {
        addVolume(ui_book_dir->totalTopItems(), "");
    });
    connect(ui_new_chapter, &FlatButton::pressed, this, &EditPage::createAndOpenNewChapter);
    connect(ui_export_to_local, &FlatButton::pressed, this, &EditPage::requestExportToLocal);
    connect(&sec_timer_, &QTimer::timeout, this, [this] {
        ui_datetime->setText(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"));
    });
    connect(ui_editor, &Editor::focusLost, this, [this] {
        messy_input_->kill();
    });
    connect(ui_book_dir, &TwoLevelTree::contextMenuRequested, this, &EditPage::popupBookDirMenu);
}

void EditPage::popupBookDirMenu(QPoint pos, TwoLevelTree::ItemInfo item_info) {
    //! TODO: menu style & popup input to edit the new title
}

void EditPage::requestExportToLocal() {
    Q_ASSERT(book_manager_);

    const auto  caption      = "导出到本地";
    const auto &title        = book_manager_->info_ref().title;
    const auto  default_name = title.isEmpty() ? "未命名书籍" : title;

    QMap<QString, ExportType> types;
    types[QStringLiteral("文本文件 (*.txt)")]     = ExportType::PlainText;
    types[QStringLiteral("EPUB 电子书 (*.epub)")] = ExportType::ePub;

    QString selected_type;
    auto    path = QFileDialog::getSaveFileName(
        this, caption, default_name, types.keys().join("\n"), &selected_type);

    if (path.isEmpty()) { return; }

    const auto type = types[selected_type];

    if (QFileInfo(path).suffix().isEmpty()) {
        switch (type) {
            case ExportType::PlainText: {
                path.append(".txt");
            } break;
            case ExportType::ePub: {
                path.append(".epub");
            } break;
        }
    }

    exportToLocal(path, type);
}

void EditPage::updateWordsCount(const QString &text) {
    const int words_diff  = text.length() - chap_words_;
    chap_words_          += words_diff;
    total_words_         += words_diff;
    syncWordsStatus();
}

void EditPage::flushWordsCount() {
    chap_words_  = 0;
    total_words_ = 0;
    if (book_manager_) {
        for (const auto cid : book_manager_->get_all_chapters()) {
            const int count = book_manager_->fetch_chapter_content(cid).value().length();
            if (cid == current_cid_) { chap_words_ = count; }
            total_words_ += count;
        }
    }
    syncWordsStatus();
}

void EditPage::syncWordsStatus() {
    ui_total_words->setText(QString("全书共 %1 字 本章 %2 字").arg(total_words_).arg(chap_words_));
}

void EditPage::createAndOpenNewChapter() {
    //! TODO: consider to create the new chapter under the volume which is corresponded to the
    //! focused top item in book dir

    if (ui_book_dir->totalTopItems() == 0) { addVolume(0, "默认卷"); }
    const int volume_index = ui_book_dir->totalTopItems() - 1;
    const int cid          = addChapter(volume_index, "");
    openChapter(cid);
    ui_book_dir->setSubItemSelected(ui_book_dir->topItemAt(volume_index), cid);
}

void EditPage::requestRenameTocItem() {
    if (!book_manager_) { return; }
    const int cid = ui_book_dir->selectedSubItem();
    if (cid == -1) { return; }

    for (const int vid : book_manager_->get_volumes()) {
        if (!book_manager_->get_chapters_of_volume(vid).contains(cid)) { continue; }
        emit renameTocItemRequested(book_manager_->info_ref(), vid, cid);
    }
}

bool EditPage::eventFilter(QObject *watched, QEvent *event) {
    if (event->type() == QEvent::KeyPress) {
        auto e = static_cast<QKeyEvent *>(event);

        //! NOTE: block special keys in messy input mode to avoid unexpceted behavior
        //! ATTENTION: this can not block global shortcut keys
        if (const auto key = QKeyCombination::fromCombined(e->key() | e->modifiers());
            !TextInputCommandManager::is_printable_char(key) && messy_input_->is_running()) {
            return true;
        }

        if (auto opt = command_manager_.match(e)) {
            const auto action = *opt;
            qDebug() << "COMMAND" << magic_enum::enum_name(action).data();
            switch (action) {
                case GlobalCommand::ToggleSidebar: {
                    ui_sidebar->setVisible(!ui_sidebar->isVisible());
                } break;
                case GlobalCommand::ToggleSoftCenterMode: {
                    ui_editor->setSoftCenterMode(!ui_editor->softCenterMode());
                } break;
                case GlobalCommand::CreateNewChapter: {
                    createAndOpenNewChapter();
                } break;
                case GlobalCommand::Rename: {
                    requestRenameTocItem();
                } break;
                case GlobalCommand::DEV_EnableMessyInput: {
                    ui_editor->setFocus();
                    messy_input_->start();
                } break;
            }
            return true;
        }
    }

    if (event->type() == QEvent::MouseButtonDblClick) {
        auto e = static_cast<QMouseEvent *>(event);
        if (e->buttons() == Qt::RightButton) {
            messy_input_->kill();
            return false;
        }
    }

    return false;
}

} // namespace jwrite::ui
