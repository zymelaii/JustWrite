#include <widget-kit/ColorPreviewItem.h>
#include <QColorDialog>
#include <QPainter>

namespace widgetkit {

ColorPreviewItem::ColorPreviewItem(QWidget *parent)
    : ClickableLabel(parent) {
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    connect(this, &ColorPreviewItem::clicked, this, &ColorPreviewItem::showColorPicker);
}

ColorPreviewItem::~ColorPreviewItem() {}

void ColorPreviewItem::setColor(const QColor &color) {
    if (color_ != color) {
        color_ = color;
        emit colorChanged(color_);
    }
    update();
}

void ColorPreviewItem::showColorPicker() {
    const auto new_color = delegate_.isNull() ? QColorDialog::getColor(color_, this, text())
                                              : delegate_->getColor(color_, this, text());
    if (new_color.isValid()) { setColor(new_color); }
}

void ColorPreviewItem::paintEvent(QPaintEvent *event) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    p.setPen(QColor(40, 40, 40));
    p.drawRect(QRectF(0.5f, 0.5f, width() - 1.0f, height() - 1.0f));
    p.fillRect(rect().adjusted(2, 2, -2, -2), color_);
}

} // namespace widgetkit
