#include <jwrite/ui/ScrollArea.h>
#include <jwrite/ui/ScrollBar.h>

namespace jwrite::ui {

ScrollArea::ScrollArea(QWidget *parent)
    : QScrollArea(parent) {
    setFrameShape(QFrame::NoFrame);

    setAutoFillBackground(true);
    setBackgroundRole(QPalette::Base);

    setWidgetResizable(true);

    setHorizontalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAlwaysOff);
    setVerticalScrollBar(new ScrollBar(this));
}

} // namespace jwrite::ui
