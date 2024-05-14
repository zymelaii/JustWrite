#include "TitleBar.h"
#include <QPainter>
#include <QPainter>

namespace jwrite {

TitleBar::TitleBar(QWidget *parent)
    : QWidget(parent) {
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
}

TitleBar::~TitleBar() {}

QSize TitleBar::sizeHint() const {
    return minimumSizeHint();
}

QSize TitleBar::minimumSizeHint() const {
    const auto margins = contentsMargins();
    const auto fm      = fontMetrics();
    const int  height  = fm.height() + fm.descent() + 8 + margins.top() + margins.bottom();
    return QSize(0, height);
}

void TitleBar::paintEvent(QPaintEvent *event) {
    QPainter   p(this);
    const auto pal = palette();

    p.fillRect(rect(), pal.window());

    p.setPen(pal.windowText().color());
    p.drawText(contentsRect(), Qt::AlignCenter, title_);
}

void TitleBar::resizeEvent(QResizeEvent *event) {
    update();
}

} // namespace jwrite
