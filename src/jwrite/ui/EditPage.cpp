#include <jwrite/ui/EditPage.h>
#include <jwrite/ui/ScrollArea.h>
#include <jwrite/TextInputCommand.h>
#include <jwrite/ProfileUtils.h>
#include <jwrite/epub/EpubBuilder.h>
#include <jwrite/AppConfig.h>
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

using namespace widgetkit;

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

    int top_item_at(int index) const override {
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

class BookDirItemRenderProxy : public TwoLevelTreeItemRenderProxy {
public:
    void render(QPainter *p, const QRect &clip_bb, const ItemInfo &item_info) {
        const auto fm    = p->fontMetrics();
        const auto flags = Qt::AlignLeft | Qt::AlignVCenter;
        const auto format =
            item_info.is_top_item ? QStringLiteral("第 %1 卷 %2") : QStringLiteral("第 %1 章 %2");
        const auto title = format.arg(item_info.level_index + 1).arg(item_info.value);
        const auto text  = fm.elidedText(title, Qt::ElideRight, clip_bb.width());
        p->drawText(clip_bb, flags, text);
    }
};

void EditPage::request_rename_toc_item(int vid, int cid) {
    Q_ASSERT(book_manager_);
    emit on_request_rename_toc_item(book_manager_->info_ref(), vid, cid);
}

void EditPage::request_invalidate_wcstate() {
    ui_word_count_->set_text("统计中");
}

void EditPage::request_sync_wcstate() {
    ui_word_count_->set_text(QString("字数 %1").arg(get_friendly_word_count(chap_words_)));
}

void EditPage::do_update_wcstate(const QString &text, bool text_changed) {
    jwrite_profiler_start(WordCounterCost);
    if (text_changed) { total_words_ -= chap_words_; }
    chap_words_ = word_counter_->count_all(text);
    if (text_changed) { total_words_ += chap_words_; }
    jwrite_profiler_record(WordCounterCost);
}

void EditPage::do_flush_wcstate() {
    chap_words_  = 0;
    total_words_ = 0;
    if (book_manager_) {
        jwrite_profiler_start(WordCounterCost);
        for (const auto cid : book_manager_->get_all_chapters()) {
            const int count =
                word_counter_->count_all(book_manager_->fetch_chapter_content(cid).value());
            if (cid == current_cid_) { chap_words_ = count; }
            total_words_ += count;
        }
        jwrite_profiler_record(WordCounterCost);
    }
}

void EditPage::do_open_chapter(int cid) {
    Q_ASSERT(book_manager_);

    if (cid == current_cid_) { return; }

    jwrite_profiler_start(SwitchChapter);

    const int next_cid = cid;
    const int last_cid = current_cid_;

    auto text = book_manager_->has_chapter(next_cid)
                  ? book_manager_->fetch_chapter_content(next_cid).value()
                  : QString{};

    if (last_cid != -1) {
        const auto loc = ui_editor_->currentTextLoc();
        if (loc.block_index != -1) { chapter_locs_[last_cid] = loc; }
    }

    do_update_wcstate(text, false);
    ui_editor_->reset(text, true);

    book_manager_->sync_chapter_content(last_cid, text);

    if (chapter_locs_.contains(next_cid)) {
        last_loc_ = chapter_locs_[next_cid];
    } else {
        last_loc_.block_index = -1;
    }

    current_cid_ = next_cid;

    //! TODO: provide interface in AbstractBookManager to get volume id of a chapter
    for (const int vid : book_manager_->get_volumes()) {
        if (!book_manager_->get_chapters_of_volume(vid).contains(cid)) { continue; }
        ui_book_dir_->setSubItemSelected(vid, cid);
        ui_book_dir_->setTopItemEllapsed(vid, false);
    }

    jwrite_profiler_record(SwitchChapter);

    //! TODO: scroll book dir to the selected chapter
}

QString EditPage::get_friendly_word_count(int count) {
    if (count > 1000 * 100) {
        return " " + QString::number(count * 1e-4, 'f', 2) + " 万";
    } else if (count > 1000 * 10) {
        return " " + QString::number(count * 1e-3, 'f', 1) + " 千";
    } else {
        return " " + QString::number(count) + " ";
    }
}

void EditPage::update_color_scheme(const ColorScheme &scheme) {
    auto pal = palette();
    pal.setColor(QPalette::Window, scheme.window());
    pal.setColor(QPalette::WindowText, scheme.window_text());
    pal.setColor(QPalette::Base, scheme.text_base());
    pal.setColor(QPalette::Text, scheme.text());
    pal.setColor(QPalette::Highlight, scheme.selected_text());
    pal.setColor(QPalette::Button, scheme.window());
    pal.setColor(QPalette::ButtonText, scheme.window_text());
    setPalette(pal);

    if (auto w = ui_named_widgets_.value("sidebar.splitter")) {
        auto pal = w->palette();
        pal.setColor(QPalette::Window, scheme.text_base());
        w->setPalette(pal);
    }

    if (auto w = ui_sidebar_) {
        auto pal = w->palette();
        pal.setColor(QPalette::Window, scheme.panel());
        w->setPalette(pal);
    }

    if (auto w = ui_editor_) {
        auto pal = w->palette();
        pal.setColor(QPalette::Window, scheme.text_base());
        pal.setColor(QPalette::HighlightedText, scheme.selected_item());
        w->setPalette(pal);
    }

    if (auto w = ui_named_widgets_.value("sidebar.button-line")) {
        auto pal = w->palette();
        pal.setColor(QPalette::Base, scheme.panel());
        pal.setColor(QPalette::Button, scheme.hover());
        w->setPalette(pal);
    }

    if (auto w = ui_named_widgets_.value("sidebar.sep-line")) {
        auto pal = w->palette();
        pal.setColor(QPalette::WindowText, scheme.border());
        w->setPalette(pal);
    }

    if (auto w = static_cast<QScrollArea *>(ui_named_widgets_.value("sidebar.book-dir-container"))
                     ->verticalScrollBar()) {
        auto pal = w->palette();
        pal.setColor(w->backgroundRole(), scheme.panel());
        pal.setColor(w->foregroundRole(), scheme.window());
        w->setPalette(pal);
    }

    if (auto w = ui_book_dir_) {
        auto pal = w->palette();
        pal.setColor(QPalette::Highlight, scheme.selected_item());
        pal.setColor(QPalette::HighlightedText, scheme.hover());
        w->setPalette(pal);
        ui_book_dir_->reloadIndicator();
    }

    if (auto w = ui_menu_) {
        w->set_background_color(scheme.floating_item());
        w->set_border_color(scheme.floating_item_border());
        w->set_hover_color(scheme.hover());
        w->reload_icon();
    }

    if (auto w = ui_word_count_) {
        auto pal = w->palette();
        pal.setColor(QPalette::Window, scheme.floating_item());
        pal.setColor(QPalette::Base, scheme.floating_item_border());
        pal.setColor(QPalette::WindowText, scheme.text());
        w->setPalette(pal);
    }
}

QString EditPage::get_book_id_of_source() const {
    return book_manager_ ? book_manager_->info_ref().uuid : "";
}

void EditPage::drop_source_ref() {
    if (!book_manager_) { return; }

    book_manager_ = nullptr;
    chapter_locs_.clear();
    current_cid_ = -1;

    total_words_ = 0;
    chap_words_  = 0;
}

bool EditPage::reset_source(AbstractBookManager *book_manager) {
    Q_ASSERT(book_manager);

    if (book_manager == book_manager_) { return false; }

    drop_source_ref();
    book_manager_ = book_manager;

    ui_book_dir_->setModel(std::make_unique<BookModel>(book_manager_, ui_book_dir_));

    current_cid_ = -1;

    return true;
}

int EditPage::add_volume(int index, const QString &title) {
    Q_ASSERT(book_manager_);
    Q_ASSERT(index >= 0 && index <= ui_book_dir_->totalTopItems());
    return ui_book_dir_->addTopItem(index, title);
}

int EditPage::add_chapter(int volume_index, const QString &title) {
    Q_ASSERT(book_manager_);
    Q_ASSERT(volume_index >= 0 && volume_index < ui_book_dir_->totalTopItems());
    const auto vid = ui_book_dir_->topItemAt(volume_index);
    Q_ASSERT(vid != -1);
    const int chap_index = ui_book_dir_->totalSubItemsUnderTopItem(vid);
    return ui_book_dir_->addSubItem(vid, chap_index, title);
}

void EditPage::sync_chapter_from_editor() {
    if (current_cid_ == -1) { return; }

    Q_ASSERT(book_manager_);

    //! FIXME: unsafe multi-thread sync
    ui_editor_->setTextEngineLocked(true);
    book_manager_->sync_chapter_content(current_cid_, ui_editor_->take());
    ui_editor_->setTextEngineLocked(false);

    current_cid_ = -1;
}

void EditPage::focus_editor() {
    if (ui_editor_->hasFocus()) { return; }
    if (last_loc_.block_index != -1) {
        ui_editor_->setCursorToTextLoc(last_loc_);
    } else {
        ui_editor_->scrollToStart();
    }
    ui_editor_->raise();
    ui_editor_->activateWindow();
    ui_editor_->setFocus();
    // Q_ASSERT(ui_editor_->hasFocus());
}

void EditPage::rename_toc_item(int id, const QString &title) {
    Q_ASSERT(book_manager_);
    ui_book_dir_->setItemValue(id, title);
}

void EditPage::create_and_open_chapter(int vid) {
    //! TODO: consider to create the new chapter under the volume which is corresponded to the
    //! focused top item in book dir

    int volume_index = ui_book_dir_->totalTopItems() - 1;
    if (vid != -1) {
        Q_ASSERT(book_manager_->has_volume(vid));
        volume_index = book_manager_->get_volumes().indexOf(vid);
    }

    if (volume_index == -1) {
        add_volume(0, "默认卷");
        volume_index = 0;
    } else if (volume_index >= ui_book_dir_->totalTopItems()) {
        add_volume(volume_index, "");
    }

    const int cid = add_chapter(volume_index, "");
    do_open_chapter(cid);
    request_sync_wcstate();
    ui_editor_->update();
    focus_editor();
    ui_book_dir_->setSubItemSelected(ui_book_dir_->topItemAt(volume_index), cid);
}

void EditPage::handle_editor_on_activate() {
    if (current_cid_ != -1) { return; }
    if (book_manager_->get_all_chapters().empty()) {
        create_and_open_chapter(-1);
    } else {
        //! NOTE: this should be done in the caller side, not here
        //! ATTENTION: here we assume that the returned chapters are in order
        // const int cid = book_manager_->get_all_chapters().last();
        // do_open_chapter(cid);
        // focus_editor();
    }
}

void EditPage::handle_editor_on_text_change(const QString &text) {
    do_update_wcstate(text, true);
    request_sync_wcstate();
}

void EditPage::handle_editor_on_focus_lost(VisualTextEditContext::TextLoc last_loc) {
    if (current_cid_ != -1) { chapter_locs_[current_cid_] = last_loc; }
    last_loc_ = last_loc;
}

void EditPage::handle_book_dir_on_select_item(bool is_top_item, int top_item_id, int sub_item_id) {
    if (!is_top_item) {
        do_open_chapter(sub_item_id);
        request_sync_wcstate();
        ui_editor_->update();
        focus_editor();
    }
}

void EditPage::handle_book_dir_on_double_click_item(
    bool is_top_item, int top_item_id, int sub_item_id) {
    request_rename_toc_item(top_item_id, sub_item_id);
}

void EditPage::handle_book_dir_on_open_menu(QPoint pos, TwoLevelTree::ItemInfo item_info) {
    //! TODO: implement the menu for book dir
}

void EditPage::handle_on_create_volume() {
    add_volume(ui_book_dir_->totalTopItems(), "");
}

void EditPage::handle_on_create_chapter() {
    const int vid     = ui_book_dir_->focusedTopItem();
    const int item_id = ui_book_dir_->selectedItem();
    if (vid == -1) {
        Q_ASSERT(item_id == -1);
        create_and_open_chapter(-1);
    } else if (vid != item_id) {
        Q_ASSERT(item_id != -1);
        Q_ASSERT(item_id == ui_book_dir_->selectedSubItem());
        create_and_open_chapter(vid);
    } else {
        create_and_open_chapter(vid);
    }
}

void EditPage::handle_on_rename_selected_toc_item() {
    if (!book_manager_) { return; }

    const int item_id = ui_book_dir_->selectedItem();
    if (item_id == -1) { return; }

    const int cid = ui_book_dir_->selectedSubItem();

    if (cid != item_id) {
        Q_ASSERT(item_id == ui_book_dir_->focusedTopItem());
        request_rename_toc_item(item_id, -1);
        return;
    }

    //! ATTENTION: do not use focusTopItem() here to get the vid, it is not always the
    //! corresponding top item to of the selected sub item

    for (const int vid : book_manager_->get_volumes()) {
        if (!book_manager_->get_chapters_of_volume(vid).contains(cid)) { continue; }
        request_rename_toc_item(vid, cid);
    }
}

EditPage::EditPage(QWidget *parent)
    : QWidget(parent)
    , current_cid_{-1}
    , chap_words_{0}
    , total_words_{0}
    , word_counter_{std::make_unique<StrictWordCounter>()}
    , book_manager_{nullptr} {
    init();
    request_invalidate_wcstate();
    last_loc_.block_index = -1;
}

void EditPage::init() {
    auto layout         = new QVBoxLayout(this);
    auto content        = new QWidget;
    auto layout_content = new QHBoxLayout(content);
    auto splitter       = new QSplitter;
    ui_named_widgets_.insert("sidebar.splitter", splitter);

    ui_editor_     = new Editor;
    ui_menu_       = new FloatingMenu(ui_editor_);
    ui_word_count_ = new FloatingLabel(ui_editor_);

    ui_new_volume_  = new FlatButton;
    ui_sidebar_     = new QWidget;
    ui_new_chapter_ = new FlatButton;
    ui_book_dir_    = new TwoLevelTree;

    auto btn_line        = new QWidget;
    auto btn_line_layout = new QHBoxLayout(btn_line);
    btn_line_layout->setContentsMargins({});
    btn_line_layout->addStretch();
    btn_line_layout->addWidget(ui_new_volume_);
    btn_line_layout->addStretch();
    btn_line_layout->addWidget(ui_new_chapter_);
    btn_line_layout->addStretch();
    ui_named_widgets_.insert("sidebar.button-line", btn_line);

    auto sep_line = new QFrame;
    sep_line->setFrameShape(QFrame::HLine);
    ui_named_widgets_.insert("sidebar.sep-line", sep_line);

    auto book_dir_container = new ScrollArea;
    book_dir_container->setWidget(ui_book_dir_);
    ui_named_widgets_.insert("sidebar.book-dir-container", book_dir_container);

    auto sidebar_layout = new QVBoxLayout(ui_sidebar_);
    sidebar_layout->setContentsMargins(0, 10, 0, 10);
    sidebar_layout->addWidget(btn_line);
    sidebar_layout->addWidget(sep_line);
    sidebar_layout->addWidget(book_dir_container);

    ui_sidebar_->setAutoFillBackground(true);
    ui_sidebar_->setMinimumWidth(200);

    splitter->addWidget(ui_sidebar_);
    splitter->addWidget(ui_editor_);

    layout->addWidget(content);
    layout->setContentsMargins({});
    layout->setSpacing(0);

    layout_content->addWidget(splitter);
    layout_content->setContentsMargins({});

    splitter->setOrientation(Qt::Horizontal);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setCollapsible(0, false);
    splitter->setCollapsible(1, false);

    ui_new_volume_->setText("新建卷");
    ui_new_chapter_->setText("新建章");

    auto font = this->font();
    font.setPointSize(10);
    ui_sidebar_->setFont(font);

    content->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

    //! install book dir item render proxy
    ui_book_dir_->setItemRenderProxy(std::make_unique<BookDirItemRenderProxy>());

    auto quit_edit_action     = new QAction(ui_menu_);
    auto open_settings_action = new QAction(ui_menu_);

    ui_menu_->add_menu_item("menu/return", quit_edit_action);
    ui_menu_->add_menu_item("menu/settings", open_settings_action);

    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    setAutoFillBackground(true);

    connect(quit_edit_action, &QAction::triggered, this, &EditPage::on_request_quit_edit);
    connect(open_settings_action, &QAction::triggered, this, &EditPage::on_request_open_settings);
    connect(ui_editor_, &Editor::activated, this, &EditPage::handle_editor_on_activate);
    connect(ui_editor_, &Editor::textChanged, this, &EditPage::handle_editor_on_text_change);
    connect(
        ui_book_dir_, &TwoLevelTree::itemSelected, this, &EditPage::handle_book_dir_on_select_item);
    connect(ui_new_volume_, &FlatButton::pressed, this, &EditPage::handle_on_create_volume);
    connect(ui_new_chapter_, &FlatButton::pressed, this, &EditPage::handle_on_create_chapter);
    connect(ui_editor_, &Editor::focusLost, this, &EditPage::handle_editor_on_focus_lost);
    connect(
        ui_book_dir_,
        &TwoLevelTree::contextMenuRequested,
        this,
        &EditPage::handle_book_dir_on_open_menu);
    connect(
        ui_book_dir_,
        &TwoLevelTree::itemDoubleClicked,
        this,
        &EditPage::handle_book_dir_on_double_click_item);
}

} // namespace jwrite::ui
