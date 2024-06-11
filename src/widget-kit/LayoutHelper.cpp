#include <widget-kit/LayoutHelper.h>
#include <QLayout>
#include <QWidget>

namespace widgetkit {

void force_update_geometry(QWidget *widget) {
    for (int i = 0; i < widget->children().size(); i++) {
        if (auto child = widget->children()[i]; child->isWidgetType()) {
            force_update_geometry(qobject_cast<QWidget *>(child));
        }
    }
    if (widget->layout()) { force_redo_layout(widget->layout()); }
}

void force_redo_layout(QLayout *layout) {
    for (int i = 0; i < layout->count(); i++) {
        if (auto item = layout->itemAt(i); item->layout()) {
            force_redo_layout(item->layout());
        } else {
            item->invalidate();
        }
    }
    layout->invalidate();
    layout->activate();
}

} // namespace widgetkit
