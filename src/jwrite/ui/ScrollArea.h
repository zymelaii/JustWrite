#include <QScrollArea>

namespace jwrite::ui {

class ScrollArea : public QScrollArea {
    Q_OBJECT

public:
    explicit ScrollArea(QWidget *parent = nullptr);
};

} // namespace jwrite::ui
