#include <jwrite/ui/FloatingDialog.h>
#include <QPainter>

namespace jwrite::ui {

FloatingDialog::FloatingDialog(QWidget *parent)
    : QWidget(parent) {
    setupUi();
}

FloatingDialog::~FloatingDialog() {}

QWidget *FloatingDialog::take() {
    auto w = ui_layout_->isEmpty() ? nullptr : ui_layout_->takeAt(0)->widget();
    //! force to recalculate the size hint
    ui_layout_->invalidate();
    return w;
}

QWidget *FloatingDialog::setWidget(QWidget *widget) {
    auto last_widget = take();
    if (widget) { ui_layout_->addWidget(widget); }
    return last_widget;
}

void FloatingDialog::setupUi() {
    ui_layout_ = new QGridLayout(this);

    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
}

void FloatingDialog::paintEvent(QPaintEvent *event) {
    QPainter p(this);

    p.fillRect(rect(), QColor(0, 0, 0, 100));
}

} // namespace jwrite::ui
