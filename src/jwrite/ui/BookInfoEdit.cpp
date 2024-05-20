#include <jwrite/ui/BookInfoEdit.h>
#include <QVBoxLayout>
#include <QPainter>

namespace jwrite::ui {

BookInfoEdit::BookInfoEdit(QWidget *parent)
    : QWidget(parent) {
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

    const auto cover = QPixmap::fromImage(
        image.scaled(ui_cover_->size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));

    ui_cover_->setPixmap(std::move(cover));
    book_info_.cover_url = cover_url;
}

void BookInfoEdit::setBookInfo(const BookInfo &info) {
    setTitle(info.title);
    setAuthor(info.author);
    setCover(info.cover_url);
}

void BookInfoEdit::setupUi() {
    ui_title_edit_   = new QtMaterialTextField;
    ui_author_edit_  = new QtMaterialTextField;
    ui_cover_        = new QLabel;
    ui_cover_select_ = new QtMaterialRaisedButton;
    ui_submit_       = new QtMaterialRaisedButton;
    ui_cancel_       = new QtMaterialRaisedButton;

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
    layout_btn->addWidget(ui_submit_);
    layout_btn->addWidget(ui_cancel_);

    ui_title_edit_->setLabel("书名");
    ui_title_edit_->setPlaceholderText("请输入书名");

    ui_author_edit_->setLabel("作者");
    ui_author_edit_->setPlaceholderText("请输入作者");

    ui_cover_->setFixedSize(150, 200);
    ui_cover_->setMargin(0);
    ui_cover_->setFrameShape(QFrame::Box);

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

    auto font = this->font();

    ui_title_edit_->setFont(font);
    ui_author_edit_->setFont(font);
    ui_cover_select_->setFont(font);
    ui_submit_->setFont(font);
    ui_cancel_->setFont(font);

    ui_title_edit_->setLabelFontSize(12);
    ui_author_edit_->setLabelFontSize(12);
    ui_cover_select_->setFontSize(12);
    ui_submit_->setFontSize(12);
    ui_cancel_->setFontSize(12);

    ui_cover_select_->setRippleStyle(Material::PositionedRipple);
    ui_submit_->setRippleStyle(Material::PositionedRipple);
    ui_cancel_->setRippleStyle(Material::PositionedRipple);

    ui_cover_select_->setHaloVisible(false);
    ui_submit_->setHaloVisible(false);
    ui_cancel_->setHaloVisible(false);

    setCover(":/res/default-cover.png");

    setFixedSize(480, 480);
}

void BookInfoEdit::setupConnections() {
    connect(ui_submit_, &QtMaterialRaisedButton::pressed, this, [this] {
        book_info_.title  = ui_title_edit_->text();
        book_info_.author = ui_author_edit_->text();
        emit submitRequested(book_info_);
    });
    connect(ui_cancel_, &QtMaterialRaisedButton::pressed, this, &BookInfoEdit::cancelRequested);
    connect(
        ui_cover_select_,
        &QtMaterialRaisedButton::pressed,
        this,
        &BookInfoEdit::changeCoverRequested);
}

void BookInfoEdit::paintEvent(QPaintEvent *event) {
    QPainter    p(this);
    const auto &pal = palette();

    p.setPen(pal.color(QPalette::Window));
    p.setBrush(pal.window());
    p.drawRoundedRect(rect(), 8, 8);
}

} // namespace jwrite::ui
