#include <widget-kit/ClickableLabel.h>
#include <QMouseEvent>

namespace widgetkit {

ClickableLabel::ClickableLabel(QWidget *parent, Qt::WindowFlags f)
    : QLabel(parent, f) {}

ClickableLabel::ClickableLabel(const QString &text, QWidget *parent, Qt::WindowFlags f)
    : QLabel(text, parent, f) {}

ClickableLabel::~ClickableLabel() {}

void ClickableLabel::mouseReleaseEvent(QMouseEvent *event) {
    if (contentsRect().contains(event->pos())) { emit clicked(); }
}

} // namespace widgetkit
