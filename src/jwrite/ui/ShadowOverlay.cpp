#include <jwrite/ui/ShadowOverlay.h>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPainter>

namespace jwrite::ui {

ShadowOverlay::ShadowOverlay(QWidget *parent)
    : QWidget(parent) {
    setupUi();
}

ShadowOverlay::~ShadowOverlay() {}

QWidget *ShadowOverlay::take() {
    auto w = ui_layout_->count() < 3 ? nullptr : ui_layout_->takeAt(1)->widget();
    //! force to recalculate the size hint
    ui_layout_->invalidate();
    return w;
}

QWidget *ShadowOverlay::setWidget(QWidget *widget) {
    auto last_widget = take();
    if (widget) { ui_layout_->insertWidget(1, widget); }
    return last_widget;
}

void ShadowOverlay::setupUi() {
    auto container = new QWidget;
    auto v_layout  = new QVBoxLayout(this);
    auto h_layout  = new QHBoxLayout(container);

    v_layout->addStretch();
    v_layout->addWidget(container);
    v_layout->addStretch();

    h_layout->addStretch();
    h_layout->addStretch();

    v_layout->setContentsMargins({});
    h_layout->setContentsMargins({});

    ui_layout_ = h_layout;

    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
}

void ShadowOverlay::paintEvent(QPaintEvent *event) {
    QPainter p(this);

    p.fillRect(rect(), QColor(0, 0, 0, 100));
}

} // namespace jwrite::ui
