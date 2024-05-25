#include <jwrite/ui/ColorSelectorButton.h>
#include <QColorDialog>
#include <QPainter>

namespace jwrite::ui {

ColorSelectorButton::ColorSelectorButton(QWidget *parent)
    : Label(parent) {
    connect(this, &ColorSelectorButton::pressed, this, &ColorSelectorButton::showColorSelector);
}

ColorSelectorButton::~ColorSelectorButton() {}

void ColorSelectorButton::setColor(QColor &color) {
    if (color_ != color) {
        color_ = color;
        emit colorChanged(color_);
    }
    update();
}

QColor ColorSelectorButton::color() {
    return color_;
}

void ColorSelectorButton::showColorSelector() {
    QColor color = QColorDialog::getColor(color_, this, text());
    if (color.isValid()) { setColor(color); }
}

void ColorSelectorButton::paintEvent(QPaintEvent *e) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    p.setPen(QColor(40, 40, 40));
    p.drawRect(QRectF(0.5f, 0.5f, width() - 1.0f, height() - 1.0f));
    p.fillRect(rect().adjusted(2, 2, -2, -2), color_);
}

} // namespace jwrite::ui
