#include <widget-kit/OverlayDialog.h>
#include <widget-kit/OverlaySurface.h>

namespace widgetkit {

OverlayDialog::OverlayDialog()
    : QWidget()
    , event_loop_{nullptr}
    , result_{0} {}

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

void OverlayDialog::exit(int result) {
    Q_ASSERT(event_loop_);
    setResult(result);
    event_loop_->exit();
}

int OverlayDialog::exec(OverlaySurface *surface) {
    Q_ASSERT(surface);

    surface->reload(this);
    surface->showOverlay();

    QEventLoop loop;
    event_loop_ = &loop;
    event_loop_->exec();
    event_loop_ = nullptr;

    if (this == surface->overlay()) { surface->closeOverlay(); }

    return result();
}

} // namespace widgetkit
