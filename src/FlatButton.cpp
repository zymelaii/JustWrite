#include "FlatButton.h"
#include <QPainter>
#include <QPaintEvent>

FlatButton::FlatButton(QWidget *parent)
    : QWidget(parent)
    , radius_{0}
    , alignment_{Qt::AlignLeft}
    , enter_{false} {}

QSize FlatButton::minimumSizeHint() const {
    const auto margins      = contentsMargins();
    const int  mw           = margins.left() + margins.right();
    const int  mh           = margins.top() + margins.bottom();
    const auto fm           = fontMetrics();
    const int  inner_width  = text_.isEmpty() ? fm.averageCharWidth() : fm.horizontalAdvance(text_);
    const int  inner_height = fm.height() + fm.descent() + 8;
    return QSize(inner_width + mw, inner_height + mh);
}

QSize FlatButton::sizeHint() const {
    return minimumSize();
}

void FlatButton::set_text(const QString &text) {
    text_ = text;
    update();
}

void FlatButton::set_radius(int radius) {
    radius_ = radius;
    update();
}

void FlatButton::set_text_alignment(int alignment) {
    alignment_ = alignment;
    update();
}

void FlatButton::paintEvent(QPaintEvent *e) {
    QPainter   p(this);
    const auto pal = palette();

    p.save();
    const auto brush = !enter_ ? pal.base() : pal.button();
    p.setBrush(brush);
    p.setPen(brush.color());
    p.drawRoundedRect(rect(), radius_, radius_);
    p.restore();

    const auto bb    = contentsRect();
    const auto flags = alignment_;
    p.drawText(bb, flags, text_);
}

void FlatButton::enterEvent(QEnterEvent *event) {
    setCursor(Qt::PointingHandCursor);
    enter_ = true;
    update();
}

void FlatButton::leaveEvent(QEvent *event) {
    setCursor(Qt::ArrowCursor);
    enter_ = false;
    update();
}

void FlatButton::mouseReleaseEvent(QMouseEvent *e) {
    emit pressed();
}
