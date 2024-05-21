#include <QScrollArea>

namespace jwrite::ui {

class ScrollArea : public QScrollArea {
    Q_OBJECT

public:
    explicit ScrollArea(QWidget *parent = nullptr);

public:
    void setEdgeOffset(int offset);
};

} // namespace jwrite::ui
