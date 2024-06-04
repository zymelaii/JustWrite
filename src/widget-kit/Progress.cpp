#include <widget-kit/Progress.h>
#include <widget-kit/OverlaySurface.h>
#include <qt-material/qtmaterialcircularprogress.h>
#include <QVBoxLayout>
#include <QtConcurrent/QtConcurrent>
#include <QFutureWatcher>
#include <QApplication>
#include <memory>

namespace widgetkit {

Progress::Progress()
    : OverlayDialog()
    , policy_(Progress::IgnoreSmallJob | Progress::UseMinimumDisplayTime)
    , minimum_display_time_{800}
    , small_job_criteria_{200} {
    setupUi();
}

Progress::~Progress() {}

int Progress::exec(OverlaySurface *surface) {
    Q_ASSERT(surface);

    //! NOTE: blocking-job is ignored when async-job is not set
    if (!async_job_) { return result(); }

    QFutureWatcher<void> watcher;
    connect(&watcher, &QFutureWatcher<void>::finished, this, &Progress::quit);

    const bool try_ignore_job = (policy_ & IgnoreSmallJob) && !blocking_job_;

    if (blocking_job_) {
        surface->reload(this);
        surface->showOverlay();
        std::invoke(blocking_job_.value());
    }

    auto fut = QtConcurrent::run(*async_job_);
    watcher.setFuture(fut);

    QElapsedTimer timer;

    if (try_ignore_job) {
        timer.start();
        do {
            QApplication::processEvents(QEventLoop::WaitForMoreEvents, 50);
            QApplication::processEvents(QEventLoop::AllEvents);
        } while (timer.elapsed() < small_job_criteria_);
        if (fut.isFinished()) { return result(); }
    }

    timer.restart();

    if (this != surface->overlay()) {
        surface->reload(this);
        surface->showOverlay();
    }

    QEventLoop loop;
    eventLoop() = &loop;
    eventLoop()->exec();
    eventLoop() = nullptr;

    if (this == surface->overlay()) {
        if (policy_ & UseMinimumDisplayTime) {
            while (timer.elapsed() < minimum_display_time_) {
                QApplication::processEvents(QEventLoop::AllEvents, 50);
            }
        }
        surface->closeOverlay();
    }

    return result();
}

void Progress::wait(OverlaySurface *surface, std::function<void()> job) {
    auto dialog = std::make_shared<Progress>();
    dialog->setAsyncJob(job);
    dialog->exec(surface);
}

void Progress::setupUi() {
    auto layout  = new QVBoxLayout(this);
    ui_progress_ = new QtMaterialCircularProgress;
    layout->addWidget(ui_progress_);
    layout->setContentsMargins({});
}

} // namespace widgetkit
