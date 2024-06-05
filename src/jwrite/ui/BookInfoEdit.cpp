#include <jwrite/ui/BookInfoEdit.h>
#include <QVBoxLayout>
#include <QPainter>
#include <QFileDialog>

namespace jwrite::ui {

void BookInfoEdit::set_title(const QString &title) {
    ui_title_edit_->setText(title);
    book_info_.author = title;
}

void BookInfoEdit::set_author(const QString &author) {
    ui_author_edit_->setText(author);
    book_info_.author = author;
}

void BookInfoEdit::set_cover(const QString &cover_url) {
    if (cover_url.isEmpty()) { return; }

    QImage image(cover_url);
    if (image.isNull()) { return; }

    ui_cover_->setImage(cover_url);
    book_info_.cover_url = cover_url;
}

void BookInfoEdit::set_book_info(const BookInfo &info) {
    book_info_ = info;
    set_title(info.title);
    set_author(info.author);
    set_cover(info.cover_url);
}

QString BookInfoEdit::select_cover(QWidget *parent, bool validate, QImage *out_image) {
    const auto filter = tr("BookInfoEdit.select_cover.filter");
    auto       path =
        QFileDialog::getOpenFileName(parent, tr("BookInfoEdit.select_cover.caption"), "", filter);

    if (path.isEmpty()) { return ""; }

    if (!validate) { return path; }

    QImage image(path);
    if (image.isNull()) { return ""; }

    if (out_image) { *out_image = std::move(image); }

    return path;
}

std::optional<BookInfo>
    BookInfoEdit::get_book_info(widgetkit::OverlaySurface *surface, const BookInfo &initial) {
    auto edit = std::make_unique<BookInfoEdit>();
    edit->set_book_info(initial);
    edit->exec(surface);
    if (edit->isAccepted()) {
        return {edit->book_info_};
    } else {
        return std::nullopt;
    }
}

void BookInfoEdit::handle_on_select_cover() {
    QImage     image{};
    const auto path = select_cover(this, true, &image);
    if (path.isEmpty()) { return; }
    set_cover(path);
}

void BookInfoEdit::handle_on_submit() {
    book_info_.title  = ui_title_edit_->text();
    book_info_.author = ui_author_edit_->text();
    accept();
}

BookInfoEdit::BookInfoEdit()
    : widgetkit::OverlayDialog() {
    init();
}

void BookInfoEdit::init() {
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

    ui_title_edit_->setLabel(tr("BookInfoEdit.title_edit.label"));
    ui_title_edit_->setPlaceholderText(tr("BookInfoEdit.title_edit.placeholder"));

    ui_author_edit_->setLabel(tr("BookInfoEdit.author_edit.label"));
    ui_author_edit_->setPlaceholderText(tr("BookInfoEdit.author_edit.placeholder"));

    ui_cover_select_->setText(tr("BookInfoEdit.cover_select.caption"));
    ui_submit_->setText(tr("BookInfoEdit.submit.text"));
    ui_cancel_->setText(tr("BookInfoEdit.cancel.text"));

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

    set_cover(":/res/default-cover.png");

    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    connect(ui_submit_, &widgetkit::FlatButton::pressed, this, &BookInfoEdit::handle_on_submit);
    connect(ui_cancel_, &widgetkit::FlatButton::pressed, this, &BookInfoEdit::quit);
    connect(
        ui_cover_select_,
        &widgetkit::FlatButton::pressed,
        this,
        &BookInfoEdit::handle_on_select_cover);
}

void BookInfoEdit::paintEvent(QPaintEvent *event) {
    QPainter    p(this);
    const auto &pal = palette();

    p.setPen(pal.color(QPalette::Window));
    p.setBrush(pal.window());
    p.drawRoundedRect(rect(), 8, 8);
}

} // namespace jwrite::ui
