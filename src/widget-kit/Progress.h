#pragma once

#include <widget-kit/OverlayDialog.h>
#include <functional>
#include <optional>

class QtMaterialCircularProgress;

namespace widgetkit {

class Progress : public OverlayDialog {
    Q_OBJECT

public:
    Progress();
    ~Progress() override;

public:
    void setJob(std::function<void()> job) {
        job_ = job;
    }

    int exec(OverlaySurface *surface) override;

    static void wait(OverlaySurface *surface, std::function<void()> job);

protected:
    void setupUi();

private:
    QtMaterialCircularProgress          *ui_progress_;
    std::optional<std::function<void()>> job_;
};

} // namespace widgetkit
