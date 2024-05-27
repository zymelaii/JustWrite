#pragma once

#include <widget-kit/OverlayDialog.h>
#include <functional>
#include <optional>

class QtMaterialCircularProgress;

namespace widgetkit {

class Progress : public OverlayDialog {
    Q_OBJECT

public:
    enum DisplayPolicyFlag {
        IgnoreSmallJob        = 0x01,
        UseMinimumDisplayTime = 0x02,
    };

    Q_DECLARE_FLAGS(DisplayPolicy, DisplayPolicyFlag)

    class Builder {
    public:
        Builder()
            : ptr_{std::make_unique<Progress>()} {}

        Builder &withBlockingJob(std::function<void()> blocking_job) {
            ptr_->setBlockingJob(blocking_job);
            return *this;
        }

        Builder &withAsyncJob(std::function<void()> async_job) {
            ptr_->setAsyncJob(async_job);
            return *this;
        }

        Builder &withPolicy(DisplayPolicyFlag policy, bool on) {
            ptr_->setDisplayPolicy(policy, on);
            return *this;
        }

        Builder &withMinimumDisplayTime(int time_ms) {
            ptr_->setMinimumDisplayTime(time_ms);
            return *this;
        }

        Builder &withSmallJobCriteria(int time_ms) {
            ptr_->setSmallJobCriteria(time_ms);
            return *this;
        }

        void exec(OverlaySurface *surface) {
            ptr_->exec(surface);
        }

    private:
        std::unique_ptr<Progress> ptr_;
    };

public:
    Progress();
    ~Progress() override;

public:
    void setBlockingJob(std::function<void()> blocking_job) {
        blocking_job_ = blocking_job;
    }

    void setAsyncJob(std::function<void()> async_job) {
        async_job_ = async_job;
    }

    void setDisplayPolicy(DisplayPolicyFlag policy, bool on) {
        if (on) {
            policy_ |= policy;
        } else {
            policy_ &= ~policy;
        }
    }

    void setMinimumDisplayTime(int time_ms) {
        minimum_display_time_ = qMax(0, time_ms);
    }

    void setSmallJobCriteria(int time_ms) {
        small_job_criteria_ = qMax(0, time_ms);
    }

    int exec(OverlaySurface *surface) override;

    static void wait(OverlaySurface *surface, std::function<void()> job);

protected:
    void setupUi();

private:
    QtMaterialCircularProgress          *ui_progress_;
    std::optional<std::function<void()>> blocking_job_;
    std::optional<std::function<void()>> async_job_;
    DisplayPolicy                        policy_;
    int                                  minimum_display_time_;
    int                                  small_job_criteria_;
};

} // namespace widgetkit
