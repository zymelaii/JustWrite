#pragma once

#include <widget-kit/MessageBox.h>

namespace jwrite::ui {

class MessageBox : public widgetkit::MessageBox {
public:
    enum class StandardIcon {
        Warning,
    };

public:
    static Choice show(
        widgetkit::OverlaySurface *surface,
        const QString             &caption,
        const QString             &text,
        StandardIcon               icon);

    static QString standardIconPath(StandardIcon icon);
};

} // namespace jwrite::ui
