#include <jwrite/ui/StatusBar.h>

namespace jwrite::Ui {

StatusBar::StatusBar(QWidget *parent)
    : QWidget(parent)
    , total_left_side_items_{0} {
    setupUi();
}

StatusBar::~StatusBar() {}

QLabel *StatusBar::addItem(const QString &text, bool right_side) {
    auto      item = new QLabel(text);
    const int index =
        right_side ? ui_layout_->count() - total_left_side_items_ + 1 : total_left_side_items_++;
    ui_layout_->insertWidget(index, item);
    return item;
}

QSize StatusBar::minimumSizeHint() const {
    const auto margins = contentsMargins();
    const int  height  = fontMetrics().height();
    return QSize(0, height + margins.top() + margins.bottom());
}

QSize StatusBar::sizeHint() const {
    return minimumSizeHint();
}

void StatusBar::setupUi() {
    ui_layout_ = new QHBoxLayout(this);
    ui_layout_->setContentsMargins({});
    ui_layout_->addStretch();

    auto font = this->font();
    font.setPointSize(10);
    setFont(font);

    setSpacing(20);
    setContentsMargins(12, 4, 12, 4);

    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);

    setAutoFillBackground(true);
}

void StatusBar::setSpacing(int spacing) {
    ui_layout_->setSpacing(spacing);
}

} // namespace jwrite::Ui
