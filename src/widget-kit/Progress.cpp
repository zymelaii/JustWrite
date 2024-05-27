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
    : OverlayDialog() {
    setupUi();
}

Progress::~Progress() {}

int Progress::exec(OverlaySurface *surface) {
    Q_ASSERT(surface);

    if (job_) {
        auto fut = QtConcurrent::run(*job_);

        QFutureWatcher<void> watcher;
        connect(
            &watcher, &QFutureWatcher<void>::finished, this, &Progress::quit, Qt::AutoConnection);
        watcher.setFuture(fut);

        QElapsedTimer timer;
        timer.start();

        while (timer.elapsed() < 200) {
            QApplication::processEvents(QEventLoop::WaitForMoreEvents, 50);
            QApplication::processEvents(QEventLoop::AllEvents);
        }

        if (!fut.isFinished()) {
            surface->reload(this);
            surface->showOverlay();

            timer.restart();

            QEventLoop loop;
            eventLoop() = &loop;
            eventLoop()->exec();
            eventLoop() = nullptr;

            if (this == surface->overlay()) {
                while (timer.elapsed() < 800) {
                    QApplication::processEvents(QEventLoop::AllEvents, 50);
                }
                surface->closeOverlay();
            }
        }
    }

    return result();
}

void Progress::wait(OverlaySurface *surface, std::function<void()> job) {
    auto dialog = std::make_shared<Progress>();
    dialog->setJob(job);
    dialog->exec(surface);
}

void Progress::setupUi() {
    auto layout  = new QVBoxLayout(this);
    ui_progress_ = new QtMaterialCircularProgress;
    layout->addWidget(ui_progress_);
    layout->setContentsMargins({});
}

} // namespace widgetkit
