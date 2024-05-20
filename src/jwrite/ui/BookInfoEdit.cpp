#include <jwrite/ui/BookInfoEdit.h>
#include <QLabel>
#include <QGridLayout>
#include <QPainter>

namespace jwrite::ui {

BookInfoEdit::BookInfoEdit(QWidget *parent)
    : QWidget(parent) {
    setupUi();
}

BookInfoEdit::~BookInfoEdit() {}

void BookInfoEdit::setupUi() {
    auto layout = new QGridLayout(this);

    auto label_title  = new QLabel;
    auto label_author = new QLabel;

    ui_title_edit_   = new QtMaterialTextField;
    ui_author_edit_  = new QtMaterialTextField;
    ui_cover_select_ = new QPushButton;

    layout->addWidget(label_title, 0, 0);
    layout->addWidget(ui_title_edit_, 0, 1);

    layout->addWidget(label_author, 1, 0);
    layout->addWidget(ui_author_edit_, 1, 1);

    layout->addWidget(ui_cover_select_, 2, 1);

    label_title->setText("书名");
    ui_title_edit_->setPlaceholderText("请输入书名");

    label_author->setText("作者");
    ui_author_edit_->setPlaceholderText("请输入作者");

    ui_cover_select_->setText("选择封面");

    layout->setContentsMargins(32, 32, 32, 32);

    setFixedSize(480, 400);
}

void BookInfoEdit::paintEvent(QPaintEvent *event) {
    QPainter p(this);

    p.setPen(Qt::white);
    p.setBrush(Qt::white);
    p.drawRoundedRect(rect(), 8, 8);
}

} // namespace jwrite::ui
