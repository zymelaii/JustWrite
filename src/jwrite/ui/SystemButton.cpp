#include <jwrite/ui/SystemButton.h>
#include <jwrite/AppConfig.h>
#include <QStyleOptionButton>
#include <QPainter>
#include <magic_enum.hpp>

namespace jwrite::ui {

SystemButton::SystemButton(SystemCommand command_type, QWidget *parent)
    : QPushButton(parent)
    , cmd_{command_type} {
    setupUi();
}

SystemButton::~SystemButton() {}

void SystemButton::reloadIcon() {
    auto &config = AppConfig::get_instance();

    if (cmd_ == SystemCommand::Close) {
        background_color_ = QColor(232, 17, 35);
    } else if (config.theme() == ColorTheme::Dark) {
        background_color_ = QColor(255, 255, 255, 38);
    } else if (config.theme() == ColorTheme::Light) {
        background_color_ = QColor(0, 0, 0, 38);
    } else {
        Q_UNREACHABLE();
    }

    const auto name      = magic_enum::enum_name<SystemCommand>(cmd_);
    const auto icon_path = config.icon(QString("sys/%1").arg(name.data()).toLower());
    QIcon      icon(icon_path);
    Q_ASSERT(!icon.isNull());

    setIcon(icon);
}

void SystemButton::setupUi() {
    reloadIcon();

    setMinimumWidth(50);
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::MinimumExpanding);
}

void SystemButton::paintEvent(QPaintEvent *event) {
    QPainter p(this);

    QStyleOptionButton opt;
    initStyleOption(&opt);

    opt.iconSize = QSize(10, 10);

    if (opt.state.testAnyFlags(QStyle::State_MouseOver | QStyle::State_Sunken)) {
        p.setPen(background_color_);
        p.fillRect(opt.rect, background_color_);
    }

    auto icon_bb = opt.rect;
    icon_bb.setSize(opt.iconSize);
    const auto delta = (opt.rect.size() - opt.iconSize) / 2;
    icon_bb.translate(delta.width(), delta.height());
    p.drawPixmap(icon_bb.topLeft(), opt.icon.pixmap(opt.iconSize));
}

} // namespace jwrite::ui
