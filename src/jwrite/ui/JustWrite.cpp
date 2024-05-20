#include <jwrite/ui/JustWrite.h>
#include <jwrite/ui/ScrollArea.h>
#include <jwrite/ui/BookInfoEdit.h>
#include <jwrite/ColorTheme.h>
#include <jwrite/ProfileUtils.h>
#include <QVBoxLayout>
#include <QKeyEvent>
#include <QDateTime>
#include <QPainter>
#include <QFileDialog>
#include <QMessageBox>

namespace jwrite::ui {

class InMemoryBookManager : public BookManager {
public:
    QString get_chapter(int cid) override {
        return chapters_.value(cid, "");
    }

    void write_chapter(int cid, const QString &text) override {
        chapters_[cid] = text;
    }

private:
    QMap<int, QString> chapters_;
};

JustWrite::JustWrite() {
    setupUi();
    setupConnections();
    closePopupLayer();

    switchToPage(PageType::Gallery);
    setTheme(Theme::Dark);
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

void JustWrite::setTheme(Theme theme) {
    ColorTheme color_theme;

    switch (theme) {
        case Theme::Light: {
            color_theme.Window       = QColor(255, 255, 255);
            color_theme.WindowText   = QColor(23, 24, 27);
            color_theme.Border       = QColor(200, 200, 200);
            color_theme.Panel        = QColor(228, 228, 215);
            color_theme.Text         = QColor(31, 32, 33);
            color_theme.TextBase     = QColor(241, 223, 222);
            color_theme.SelectedText = QColor(140, 180, 100);
            color_theme.Hover        = QColor(30, 30, 80, 50);
            color_theme.SelectedItem = QColor(196, 165, 146, 60);
        } break;
        case Theme::Dark: {
            color_theme.Window       = QColor(60, 60, 60);
            color_theme.WindowText   = QColor(160, 160, 160);
            color_theme.Border       = QColor(70, 70, 70);
            color_theme.Panel        = QColor(38, 38, 38);
            color_theme.Text         = QColor(255, 255, 255);
            color_theme.TextBase     = QColor(30, 30, 30);
            color_theme.SelectedText = QColor(60, 60, 255, 80);
            color_theme.Hover        = QColor(255, 255, 255, 30);
            color_theme.SelectedItem = QColor(255, 255, 255, 10);
        } break;
    }

    auto pal = palette();
    pal.setColor(QPalette::Window, color_theme.Window);
    pal.setColor(QPalette::WindowText, color_theme.WindowText);
    pal.setColor(QPalette::Base, color_theme.TextBase);
    pal.setColor(QPalette::Text, color_theme.Text);
    pal.setColor(QPalette::Highlight, color_theme.SelectedText);
    pal.setColor(QPalette::Button, color_theme.Window);
    pal.setColor(QPalette::ButtonText, color_theme.WindowText);
    setPalette(pal);

    ui_gallery_->updateColorTheme(color_theme);
    ui_edit_page_->updateColorTheme(color_theme);
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
    ui_popup_layer_ = new FloatingDialog;

    auto gallery_page = new ScrollArea;
    gallery_page->setWidget(ui_gallery_);

    ui_page_stack_->addWidget(gallery_page);
    ui_page_stack_->addWidget(ui_edit_page_);

    ui_top_most_layout_->addWidget(ui_page_stack_);
    ui_top_most_layout_->addWidget(ui_popup_layer_);

    ui_agent_ = new QWK::WidgetWindowAgent(this);
    ui_agent_->setup(this);
    ui_agent_->setTitleBar(ui_title_bar_);

    top_layout->setContentsMargins({});
    top_layout->setSpacing(0);

    container->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

    ui_top_most_layout_->setStackingMode(QStackedLayout::StackAll);
    ui_top_most_layout_->setCurrentWidget(ui_popup_layer_);

    page_map_[PageType::Gallery] = gallery_page;
    page_map_[PageType::Edit]    = ui_edit_page_;

    ui_title_bar_->setTitle("只写 丶 阐释你的梦");
}

void JustWrite::setupConnections() {
    connect(ui_gallery_, &Gallery::clicked, this, [this](int index) {
        if (index == ui_gallery_->totalItems()) { requestUpdateBookInfo(index); }
    });
    connect(ui_gallery_, &Gallery::menuClicked, this, &JustWrite::requestBookAction);
}

void JustWrite::requestUpdateBookInfo(int index) {
    Q_ASSERT(index >= 0 && index <= ui_gallery_->totalItems());
    const bool on_insert = index == ui_gallery_->totalItems();

    auto info = on_insert ? BookInfo{} : ui_gallery_->bookInfoAt(index);
    if (info.author.isEmpty()) { info.author = getLikelyAuthor(); }
    if (info.title.isEmpty()) { info.title = QString("未命名书籍-%1").arg(index + 1); }

    auto edit = new BookInfoEdit;
    edit->setBookInfo(info);

    showPopupLayer(edit);

    connect(edit, &BookInfoEdit::submitRequested, this, [this, index](BookInfo info) {
        updateBookInfo(index, info);
        closePopupLayer();
    });
    connect(edit, &BookInfoEdit::cancelRequested, this, &JustWrite::closePopupLayer);
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
    Q_ASSERT(index != ui_gallery_->totalItems());
    switch (action) {
        case Gallery::Open: {
            auto book     = new InMemoryBookManager;
            book->book_id = books_.size();
            book->info    = ui_gallery_->bookInfoAt(index);
            books_.insert(book->book_id, book);
            ui_edit_page_->resetBookSource(book);
            switchToPage(PageType::Edit);
        } break;
        case Gallery::Edit: {
            requestUpdateBookInfo(index);
        } break;
        case Gallery::Delete: {
            const auto result = QMessageBox::information(
                this, "删除书籍", "是否确认删除？", QMessageBox::Yes | QMessageBox::No);
            if (result == QMessageBox::Yes) { ui_gallery_->removeDisplayCase(index); }
        } break;
    }
}

void JustWrite::showPopupLayer(QWidget *widget) {
    ui_popup_layer_->setWidget(widget);
    ui_popup_layer_->show();
}

void JustWrite::closePopupLayer() {
    ui_popup_layer_->hide();
    if (auto w = ui_popup_layer_->take()) { delete w; }
}

void JustWrite::switchToPage(PageType page) {
    Q_ASSERT(page_map_.contains(page));
    Q_ASSERT(page_map_.value(page, nullptr));
    if (auto w = page_map_[page]; w != ui_page_stack_->currentWidget()) {
        closePopupLayer();
        ui_page_stack_->setCurrentWidget(w);
    }
}

} // namespace jwrite::ui
