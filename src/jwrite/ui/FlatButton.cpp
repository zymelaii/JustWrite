#include <jwrite/ui/FlatButton.h>
#include <QPainter>

namespace jwrite::Ui {

FlatButton::FlatButton(QWidget *parent)
    : QWidget(parent)
    , ui_radius_{0}
    , ui_alignment_{Qt::AlignLeft}
    , ui_mouse_entered_{false} {
    setupUi();
}

FlatButton::~FlatButton() {}

void FlatButton::setText(const QString &text) {
    text_ = text;
    update();
}

void FlatButton::setRadius(int radius) {
    ui_radius_ = radius;
    update();
}

void FlatButton::setTextAlignment(Qt::Alignment alignment) {
    ui_alignment_ = alignment;
    update();
}

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

void FlatButton::setupUi() {
    setContentsMargins(10, 0, 10, 0);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    setTextAlignment(Qt::AlignCenter);
    setRadius(6);
}

void FlatButton::paintEvent(QPaintEvent *e) {
    QPainter   p(this);
    const auto pal = palette();

    const auto brush = !ui_mouse_entered_ ? pal.base() : pal.button();

    p.save();
    p.setBrush(brush);
    p.setPen(Qt::transparent);
    p.drawRoundedRect(rect(), ui_radius_, ui_radius_);
    p.restore();

    const auto bb    = contentsRect();
    const auto flags = ui_alignment_;
    p.drawText(bb, flags, text_);
}

void FlatButton::enterEvent(QEnterEvent *event) {
    setCursor(Qt::PointingHandCursor);
    ui_mouse_entered_ = true;
    update();
}

void FlatButton::leaveEvent(QEvent *event) {
    setCursor(Qt::ArrowCursor);
    ui_mouse_entered_ = false;
    update();
}

void FlatButton::mouseReleaseEvent(QMouseEvent *e) {
    emit pressed();
}

} // namespace jwrite::Ui
