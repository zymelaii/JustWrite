#include <jwrite/ui/SystemButton.h>
#include <QStyleOptionButton>
#include <QPainter>

namespace jwrite::ui {

SystemButton::SystemButton(SystemCommand command_type, QWidget *parent)
    : QPushButton(parent)
    , cmd_{command_type} {
    setupUi();
}

void SystemButton::setupUi() {
    reloadIcon();

    setMinimumWidth(50);
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::MinimumExpanding);
}

void SystemButton::reloadIcon() {
    QString name{};

    switch (cmd_) {
        case SystemCommand::Close: {
            name              = "close";
            background_color_ = QColor(232, 17, 35);
        } break;
        case SystemCommand::Minimize: {
            name              = "minimize";
            background_color_ = QColor(255, 255, 255, 38);
        } break;
        case SystemCommand::Maximize: {
            name              = "maximize";
            background_color_ = QColor(255, 255, 255, 38);
        } break;
    }

    setIcon(QIcon(QString(":/res/icons/system/%1.svg").arg(name)));
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
