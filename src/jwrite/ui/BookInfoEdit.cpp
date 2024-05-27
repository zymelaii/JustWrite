#include <jwrite/ui/BookInfoEdit.h>
#include <QVBoxLayout>
#include <QPainter>
#include <QFileDialog>

namespace jwrite::ui {

BookInfoEdit::BookInfoEdit()
    : widgetkit::OverlayDialog() {
    setupUi();
    setupConnections();
}

BookInfoEdit::~BookInfoEdit() {}

void BookInfoEdit::setTitle(const QString &title) {
    ui_title_edit_->setText(title);
    book_info_.author = title;
}

void BookInfoEdit::setAuthor(const QString &author) {
    ui_author_edit_->setText(author);
    book_info_.author = author;
}

void BookInfoEdit::setCover(const QString &cover_url) {
    if (cover_url.isEmpty()) { return; }

    QImage image(cover_url);
    if (image.isNull()) { return; }

    ui_cover_->setImage(cover_url);
    book_info_.cover_url = cover_url;
}

void BookInfoEdit::setBookInfo(const BookInfo &info) {
    book_info_ = info;
    setTitle(info.title);
    setAuthor(info.author);
    setCover(info.cover_url);
}

QString BookInfoEdit::getCoverPath(QWidget *parent, bool validate, QImage *out_image) {
    const auto filter = "图片 (*.bmp *.jpg *.jpeg *.png)";
    auto       path   = QFileDialog::getOpenFileName(parent, "选择封面", "", filter);

    if (path.isEmpty()) { return ""; }

    if (!validate) { return path; }

    QImage image(path);
    if (image.isNull()) { return ""; }

    if (out_image) { *out_image = std::move(image); }

    return path;
}

std::optional<BookInfo>
    BookInfoEdit::getBookInfo(widgetkit::OverlaySurface *surface, const BookInfo &initial) {
    auto edit = std::make_unique<BookInfoEdit>();
    edit->setBookInfo(initial);
    const int request = edit->exec(surface);
    if (request == BookInfoEdit::Submit) {
        return {edit->book_info_};
    } else {
        return std::nullopt;
    }
}

void BookInfoEdit::selectCoverImage() {
    QImage     image{};
    const auto path = getCoverPath(this, true, &image);
    if (path.isEmpty()) { return; }
    setCover(path);
}

void BookInfoEdit::setupUi() {
    ui_title_edit_   = new QtMaterialTextField;
    ui_author_edit_  = new QtMaterialTextField;
    ui_cover_        = new widgetkit::ImageLabel;
    ui_cover_select_ = new widgetkit::FlatButton;
    ui_submit_       = new widgetkit::FlatButton;
    ui_cancel_       = new widgetkit::FlatButton;

    auto row_container = new QWidget;
    auto btn_container = new QWidget;

    auto layout     = new QVBoxLayout(this);
    auto layout_row = new QHBoxLayout(row_container);
    auto layout_btn = new QVBoxLayout(btn_container);

    layout->addWidget(ui_title_edit_);
    layout->addWidget(ui_author_edit_);
    layout->addWidget(row_container);

    layout_row->addWidget(ui_cover_);
    layout_row->addWidget(btn_container);

    layout_btn->addWidget(ui_cover_select_);
    layout_btn->addStretch();
    layout_btn->addWidget(ui_submit_);
    layout_btn->addStretch();
    layout_btn->addWidget(ui_cancel_);

    ui_cover_->setViewSize(QSize(150, 200));
    ui_cover_->setBorderVisible(true);

    ui_title_edit_->setLabel("书名");
    ui_title_edit_->setPlaceholderText("请输入书名");

    ui_author_edit_->setLabel("作者");
    ui_author_edit_->setPlaceholderText("请输入作者");

    ui_cover_select_->setText("选择封面");
    ui_submit_->setText("确认");
    ui_cancel_->setText("返回");

    layout->setContentsMargins(32, 32, 32, 32);
    layout_row->setSpacing(8);

    auto pal = palette();
    pal.setColor(QPalette::Window, Qt::white);
    pal.setColor(QPalette::Base, Qt::white);
    pal.setColor(QPalette::Text, Qt::black);
    setPalette(pal);
    pal.setColor(QPalette::Window, QColor("#a0a0a0"));
    ui_cover_->setPalette(pal);
    pal.setColor(QPalette::Window, QColor("#6d6d6d"));
    pal.setColor(QPalette::Base, QColor("#191919"));
    pal.setColor(QPalette::Button, QColor("#393939"));
    pal.setColor(QPalette::WindowText, QColor("#ffffff"));
    ui_cover_select_->setPalette(pal);
    ui_submit_->setPalette(pal);
    ui_cancel_->setPalette(pal);

    const int  button_radius         = 4;
    const auto btn_margins           = QMargins(16, 8, 16, 8);
    const bool button_border_visible = true;
    ui_cover_select_->setRadius(button_radius);
    ui_submit_->setRadius(button_radius);
    ui_cancel_->setRadius(button_radius);
    ui_cover_select_->setContentsMargins(btn_margins);
    ui_submit_->setContentsMargins(btn_margins);
    ui_cancel_->setContentsMargins(btn_margins);
    ui_cover_select_->setBorderVisible(button_border_visible);
    ui_submit_->setBorderVisible(button_border_visible);
    ui_cancel_->setBorderVisible(button_border_visible);
    ui_cover_select_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    ui_submit_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    ui_cancel_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    auto font = this->font();
    ui_title_edit_->setFont(font);
    ui_author_edit_->setFont(font);
    font.setPointSize(12);
    ui_cover_select_->setFont(font);
    ui_submit_->setFont(font);
    ui_cancel_->setFont(font);

    ui_title_edit_->setLabelFontSize(12);
    ui_author_edit_->setLabelFontSize(12);

    setCover(":/res/default-cover.png");

    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
}

void BookInfoEdit::setupConnections() {
    connect(ui_submit_, &widgetkit::FlatButton::pressed, this, [this] {
        book_info_.title  = ui_title_edit_->text();
        book_info_.author = ui_author_edit_->text();
        exit(Request::Submit);
    });
    connect(ui_cancel_, &widgetkit::FlatButton::pressed, this, [this] {
        exit(Request::Cancel);
    });
    connect(
        ui_cover_select_, &widgetkit::FlatButton::pressed, this, &BookInfoEdit::selectCoverImage);
}

void BookInfoEdit::paintEvent(QPaintEvent *event) {
    QPainter    p(this);
    const auto &pal = palette();

    p.setPen(pal.color(QPalette::Window));
    p.setBrush(pal.window());
    p.drawRoundedRect(rect(), 8, 8);
}

} // namespace jwrite::ui
