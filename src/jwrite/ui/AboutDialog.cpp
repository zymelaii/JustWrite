#include <jwrite/ui/AboutDialog.h>
#include <widget-kit/ImageLabel.h>
#include <widget-kit/OverlaySurface.h>
#include <QVBoxLayout>
#include <QLabel>
#include <QApplication>
#include <QPainter>
#include <QKeyEvent>
#include <memory>

using namespace widgetkit;

namespace jwrite::ui {

AboutDialog::AboutDialog()
    : OverlayDialog() {
    init();
}

int AboutDialog::exec(widgetkit::OverlaySurface* surface) {
    Q_ASSERT(surface);
    surface->installEventFilter(this);
    const int result = OverlayDialog::exec(surface);
    surface->removeEventFilter(this);
    return result;
}

void AboutDialog::show(OverlaySurface* surface) {
    auto dialog = std::make_shared<AboutDialog>();
    dialog->exec(surface);
}

void AboutDialog::init() {
    auto layout = new QVBoxLayout(this);

    auto ui_app_name    = new QLabel(QApplication::applicationDisplayName());
    auto ui_app_version = new QLabel(QString("版本 %1").arg(QApplication::applicationVersion()));

    layout->addWidget(ui_app_name);
    layout->addWidget(ui_app_version);

    ui_app_name->setAlignment(Qt::AlignCenter);
    ui_app_version->setAlignment(Qt::AlignCenter);

    layout->setAlignment(Qt::AlignCenter);
    layout->setContentsMargins(32, 16, 32, 16);

    auto font = this->font();
    font.setPointSize(16);
    ui_app_name->setFont(font);
    font.setPointSize(8);
    ui_app_version->setFont(font);

    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
}

bool AboutDialog::eventFilter(QObject* watched, QEvent* event) {
    if (event->type() == QEvent::MouseButtonPress) {
        const auto e   = static_cast<QMouseEvent*>(event);
        const auto pos = mapFromGlobal(e->globalPosition()).toPoint();
        if (!contentsRect().contains(pos)) { quit(); }
    }
    return false;
}

void AboutDialog::paintEvent(QPaintEvent* event) {
    QPainter    p(this);
    const auto& pal = palette();

    p.setPen(pal.color(QPalette::Window));
    p.setBrush(pal.window());
    p.drawRoundedRect(rect(), 8, 8);
}

void AboutDialog::keyPressEvent(QKeyEvent* event) {
    quit();
}

} // namespace jwrite::ui
