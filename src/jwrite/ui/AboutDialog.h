#pragma once

#include <widget-kit/OverlayDialog.h>

namespace jwrite::ui {

class AboutDialog : public widgetkit::OverlayDialog {
public:
    int         exec(widgetkit::OverlaySurface* surface) override;
    static void show(widgetkit::OverlaySurface* surface);

public:
    AboutDialog();

protected:
    void init();

    bool eventFilter(QObject* watched, QEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
};

} // namespace jwrite::ui
