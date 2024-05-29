#pragma once

#include <widget-kit/OverlayDialog.h>

namespace jwrite::ui {

class AboutDialog : public widgetkit::OverlayDialog {
public:
    AboutDialog();
    ~AboutDialog() override;

public:
    static void show(widgetkit::OverlaySurface* surface);

protected:
    void init();

    void paintEvent(QPaintEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
};

} // namespace jwrite::ui
