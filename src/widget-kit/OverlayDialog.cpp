#include <widget-kit/OverlayDialog.h>
#include <widget-kit/OverlaySurface.h>

namespace widgetkit {

OverlayDialog::OverlayDialog()
    : QWidget()
    , event_loop_{nullptr}
    , result_{0}
    , surface_{nullptr} {}

OverlayDialog::~OverlayDialog() {}

void OverlayDialog::accept() {
    if (!event_loop_) { return; }
    accepted_or_rejected_ = true;
    event_loop_->exit();
    emit accepted();
}

void OverlayDialog::reject() {
    if (!event_loop_) { return; }
    accepted_or_rejected_ = false;
    event_loop_->exit();
    emit rejected();
}

void OverlayDialog::quit() {
    exit(0);
}

void OverlayDialog::exit(int result) {
    setResult(result);
    if (event_loop_) { event_loop_->exit(); }
}

int OverlayDialog::exec(OverlaySurface *surface) {
    Q_ASSERT(surface);

    surface->reload(this);
    surface->showOverlay();

    surface_ = surface;

    QEventLoop loop;
    event_loop_ = &loop;
    event_loop_->exec();
    event_loop_ = nullptr;

    if (this == surface->overlay()) { surface->closeOverlay(); }

    surface_ = nullptr;

    return result();
}

} // namespace widgetkit
