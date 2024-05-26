#include <jwrite/ui/MessageBox.h>

namespace jwrite::ui {

MessageBox::Choice MessageBox::show(
    widgetkit::OverlaySurface *surface,
    const QString             &caption,
    const QString             &text,
    StandardIcon               icon) {
    return widgetkit::MessageBox::show(surface, caption, text, standardIconPath(icon));
}

QString MessageBox::standardIconPath(StandardIcon icon) {
    switch (icon) {
        case StandardIcon::Warning: {
            return ":/res/icons/warning.svg";
        } break;
    }
}

} // namespace jwrite::ui
