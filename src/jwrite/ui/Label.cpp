#include <jwrite/ui/Label.h>
#include <QMouseEvent>

namespace jwrite::ui {

Label::Label(QWidget *parent, Qt::WindowFlags f)
    : QLabel(parent, f) {}

Label::Label(const QString &text, QWidget *parent, Qt::WindowFlags f)
    : QLabel(text, parent, f) {}

Label::~Label() {}

void Label::mousePressEvent(QMouseEvent *event) {
    QLabel::mousePressEvent(event);
    if (contentsRect().contains(event->pos())) { emit pressed(); }
}

} // namespace jwrite::ui
