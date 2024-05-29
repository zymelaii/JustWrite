#include <jwrite/ui/FloatingMenu.h>
#include <jwrite/AppConfig.h>
#include <QPainter>
#include <QPainterPath>
#include <QEvent>
#include <QMouseEvent>

namespace jwrite::ui {

int FloatingMenu::get_icon_size() const {
    return 20;
}

int FloatingMenu::get_item_size() const {
    return 48;
}

void FloatingMenu::set_background_color(const QColor &color) {
    ui_background_color_ = color;
    update();
}

void FloatingMenu::set_border_color(const QColor &color) {
    ui_border_color_ = color;
    update();
}

void FloatingMenu::set_hover_color(const QColor &color) {
    ui_hover_color_ = color;
    update();
}

void FloatingMenu::update_geometry() {
    if (auto w = parentWidget()) {
        const auto size    = sizeHint();
        const int  spacing = 32;
        const auto rect    = w->contentsRect();
        const auto top_left =
            rect.bottomRight() - QPoint(spacing + size.width(), spacing + size.height());
        setGeometry(QRect(top_left, size));
    } else {
        setGeometry(0, 0, 0, 0);
    }
}

void FloatingMenu::reload_icon() {
    auto     &config    = AppConfig::get_instance();
    const int icon_size = get_icon_size();
    ui_menu_icon_       = QIcon(config.icon("menu/more")).pixmap(icon_size);
    for (int i = 0; i < menu_items_.size(); ++i) {
        ui_menu_items_[i] = QIcon(config.icon(menu_items_[i].icon_name)).pixmap(icon_size);
    }
    update();
}

void FloatingMenu::set_expanded(bool expanded) {
    ui_expanded_ = expanded;
    update_geometry();
}

void FloatingMenu::add_menu_item(const QString &icon_name, QAction *action) {
    Q_ASSERT(action);
    auto     &config    = AppConfig::get_instance();
    const int icon_size = get_icon_size();
    menu_items_.append({icon_name, action});
    ui_menu_items_.append(QIcon(config.icon(icon_name)).pixmap(icon_size));
    if (ui_expanded_) { update_geometry(); }
}

int FloatingMenu::get_index_of_menu_item(const QPoint &pos) {
    if (!ui_expanded_ || menu_items_.empty()) { return -1; }
    const auto bb = contentsRect();
    if (!bb.contains(pos)) { return -1; }
    const int last_index = menu_items_.size() - 1;
    const int rev_index  = qBound(0, (pos.y() - bb.top()) / get_item_size(), last_index);
    return last_index - rev_index;
}

FloatingMenu::FloatingMenu(QWidget *parent)
    : QWidget(parent)
    , ui_expanded_{false}
    , ui_background_color_(Qt::black)
    , ui_border_color_(Qt::white) {
    Q_ASSERT(parent);

    setupUi();
    setMouseTracking(true);
    setAttribute(Qt::WA_Hover);

    parent->installEventFilter(this);
    update_geometry();
}

FloatingMenu::~FloatingMenu() {
    for (auto &[_, action] : menu_items_) {
        if (!action->parent()) { action->deleteLater(); }
    }
}

QSize FloatingMenu::minimumSizeHint() const {
    if (!parent()) { return QSize(0, 0); }
    const auto  margins = contentsMargins();
    const QSize margin_size(margins.left() + margins.right(), margins.top() + margins.bottom());
    const int   item_extent = get_item_size();
    QSize       item_size(item_extent, item_extent);
    if (!ui_expanded_) { return item_size + margin_size; }
    item_size.setHeight(item_extent * qMax(1, ui_menu_items_.size()));
    return item_size + margin_size;
}

QSize FloatingMenu::sizeHint() const {
    return minimumSizeHint();
}

void FloatingMenu::setupUi() {
    reload_icon();

    setContentsMargins(4, 4, 4, 4);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
}

bool FloatingMenu::eventFilter(QObject *obj, QEvent *event) {
    switch (event->type()) {
        case QEvent::Move:
            [[fallthrough]];
        case QEvent::Resize: {
            update_geometry();
        } break;
        case QEvent::MouseButtonPress:
            [[fallthrough]];
        case QEvent::MouseButtonRelease:
            [[fallthrough]];
        case QEvent::MouseButtonDblClick:
            [[fallthrough]];
        case QEvent::MouseMove: {
            if (ui_expanded_) { return true; }
        } break;
        default: {
        } break;
    }
    return QWidget::eventFilter(obj, event);
}

bool FloatingMenu::event(QEvent *event) {
    if (!parent()) { return QWidget::event(event); }
    switch (event->type()) {
        case QEvent::ParentChange: {
            parent()->installEventFilter(this);
            update_geometry();
            break;
        }
        case QEvent::ParentAboutToChange: {
            parent()->removeEventFilter(this);
            break;
        }
        default:
            break;
    }
    return QWidget::event(event);
}

void FloatingMenu::paintEvent(QPaintEvent *event) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const auto  bb          = contentsRect();
    const auto  item_extent = get_item_size();
    const QSize item_size   = QSize(item_extent, item_extent);
    const int   icon_size   = get_icon_size();
    auto        item_bb     = QRect(bb.bottomRight() - QPoint(item_extent, item_extent), item_size);

    p.setBrush(ui_background_color_);
    p.setPen(ui_border_color_);

    QPainterPath clip_path;
    if (!ui_expanded_) {
        clip_path.addEllipse(item_bb);
    } else {
        clip_path.addRoundedRect(bb, item_extent / 2, item_extent / 2);
    }
    p.drawPath(clip_path);

    auto pos = QPointF(item_bb.x() + item_bb.width() * 0.5, item_bb.y() + item_bb.height() * 0.5)
             - QPointF(icon_size * 0.5, icon_size * 0.5);
    if (ui_expanded_) {
        if (ui_hover_on_ != -1) { p.setClipPath(clip_path); }
        int index = 0;
        for (const auto &icon : ui_menu_items_) {
            if (index == ui_hover_on_) { p.fillRect(item_bb, ui_hover_color_); }
            p.drawPixmap(pos, icon);
            pos -= QPointF(0, item_extent);
            item_bb.translate(0, -item_extent);
            ++index;
        }
    } else {
        p.drawPixmap(pos, ui_menu_icon_);
    }
}

void FloatingMenu::enterEvent(QEnterEvent *event) {
    set_expanded(true);
    QWidget::enterEvent(event);
}

void FloatingMenu::leaveEvent(QEvent *event) {
    set_expanded(false);
    ui_hover_on_ = -1;
    QWidget::leaveEvent(event);
}

void FloatingMenu::mouseMoveEvent(QMouseEvent *event) {
    const int last_index = ui_hover_on_;
    ui_hover_on_         = get_index_of_menu_item(event->pos());
    if (ui_hover_on_ != last_index) { update(); }
}

void FloatingMenu::mousePressEvent(QMouseEvent *event) {
    if (ui_hover_on_ != -1) {
        Q_ASSERT(ui_expanded_);
        Q_ASSERT(ui_hover_on_ >= 0 && ui_hover_on_ < menu_items_.size());
        menu_items_[ui_hover_on_].action->trigger();
    }
}

} // namespace jwrite::ui
