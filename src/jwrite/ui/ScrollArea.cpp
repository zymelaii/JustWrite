#include <jwrite/ui/ScrollArea.h>
#include <jwrite/ui/ScrollBar.h>

namespace jwrite::ui {

ScrollArea::ScrollArea(QWidget *parent)
    : QScrollArea(parent) {
    setFrameShape(QFrame::NoFrame);

    setHorizontalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAlwaysOff);
    setVerticalScrollBar(new ScrollBar(this));

    setWidgetResizable(true);

    setAutoFillBackground(false);
}

void ScrollArea::setEdgeOffset(int offset) {
    static_cast<ScrollBar *>(verticalScrollBar())->setEdgeOffset(offset);
}

} // namespace jwrite::ui
