#include <jwrite/ui/ToolbarIcon.h>
#include <jwrite/AppConfig.h>
#include <QHelpEvent>
#include <QToolTip>
#include <QPainter>

using namespace widgetkit;

namespace jwrite::ui {

int ToolbarIcon::get_icon_size() const {
    return icon_size_;
}

void ToolbarIcon::set_icon_size(int size) {
    Q_ASSERT(size > 0);
    icon_size_ = size;
    adjustSize();
    reload_icon();
}

void ToolbarIcon::set_icon(const QString &icon_name) {
    Q_ASSERT(!icon_name.isEmpty());
    icon_name_ = icon_name;
    reload_icon();
}

void ToolbarIcon::reload_icon() {
    const auto icon_path = AppConfig::get_instance().icon(icon_name_);
    QIcon      icon(icon_path);
    Q_ASSERT(!icon.isNull());
    ui_icon_ = icon.pixmap(icon_size_, icon_size_);
    update();
}

void ToolbarIcon::set_action(QAction *action) {
    if (action_ == action) { return; }
    reset_action();
    if (action) { connect(this, &ToolbarIcon::clicked, action, &QAction::trigger); }
    action_ = action;
}

void ToolbarIcon::reset_action() {
    if (!action_) { return; }
    disconnect(this, &ToolbarIcon::clicked, action_, nullptr);
    if (auto parent = action_->parent(); !parent || parent == this) { action_->deleteLater(); }
    action_ = nullptr;
}

void ToolbarIcon::set_tool_tip(const QString &tip) {
    setToolTip(tip);
}

void ToolbarIcon::trigger() {
    if (action_) { action_->trigger(); }
}

ToolbarIcon::ToolbarIcon(const QString &icon_name, QWidget *parent)
    : ClickableLabel(parent)
    , action_{nullptr}
    , icon_size_{12}
    , ui_hover_{false} {
    set_icon(icon_name);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
}

ToolbarIcon::~ToolbarIcon() {
    reset_action();
}

QSize ToolbarIcon::minimumSizeHint() const {
    const int padding = 6;
    return QSize(icon_size_ + padding * 2, icon_size_ + padding * 2);
}

QSize ToolbarIcon::sizeHint() const {
    return minimumSizeHint();
}

bool ToolbarIcon::event(QEvent *event) {
    if (event->type() == QEvent::ToolTip) {
        const auto tool_tip = toolTip();
        if (tool_tip.isEmpty()) {
            event->ignore();
        } else {
            const auto pos = mapToGlobal(QPoint(width() + 8, 0));
            QToolTip::showText(pos, tool_tip);
        }
        return true;
    }
    return ClickableLabel::event(event);
}

void ToolbarIcon::enterEvent(QEnterEvent *event) {
    ui_hover_ = true;
    update();
    ClickableLabel::enterEvent(event);
}

void ToolbarIcon::leaveEvent(QEvent *event) {
    ui_hover_ = false;
    update();
    ClickableLabel::leaveEvent(event);
}

void ToolbarIcon::paintEvent(QPaintEvent *event) {
    if (ui_icon_.isNull()) { return; }

    QPainter    p(this);
    const auto &pal = palette();

    const auto bb     = contentsRect();
    const int  extent = get_icon_size();

    if (ui_hover_) {
        p.setPen(Qt::transparent);
        p.setBrush(pal.window());
        p.drawRoundedRect(bb, 4, 4);
    }

    const auto pos = QPointF(bb.width() - extent, bb.height() - extent) / 2;
    p.drawPixmap(pos, ui_icon_);
}

} // namespace jwrite::ui
