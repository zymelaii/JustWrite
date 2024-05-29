#include <jwrite/ui/Toolbar.h>
#include <jwrite/ui/ToolbarIcon.h>

namespace jwrite::ui {

void Toolbar::update_color_scheme(const ColorScheme &scheme) {
    auto pal = palette();
    pal.setColor(backgroundRole(), scheme.text_base());
    pal.setColor(QPalette::Window, scheme.selected_item());
    setPalette(pal);
    reload_toolbar_icons();
}

void Toolbar::reload_toolbar_icons() {
    const int total_items = ui_layout_->count();
    for (int i = 0; i < total_items; i++) {
        auto w = ui_layout_->itemAt(i)->widget();
        if (!w) { continue; }
        auto toolbar_icon = qobject_cast<ToolbarIcon *>(w);
        Q_ASSERT(toolbar_icon);
        toolbar_icon->reload_icon();
    }
}

void Toolbar::add_item(
    const QString &tip, const QString &icon_name, QAction *action, bool bottom_side) {
    auto toolbar_icon = new ToolbarIcon(icon_name);
    toolbar_icon->set_tool_tip(tip);
    toolbar_icon->set_action(action);
    if (bottom_side) {
        ui_layout_->addWidget(toolbar_icon);
        ++ui_total_bottom_side_;
    } else {
        const int index = ui_layout_->count() - ui_total_bottom_side_ - 1;
        ui_layout_->insertWidget(index, toolbar_icon);
    }
}

Toolbar::Toolbar(QWidget *parent)
    : QWidget(parent)
    , ui_total_bottom_side_{0}
    , ui_layout_{nullptr} {
    init();
}

void Toolbar::init() {
    ui_layout_ = new QVBoxLayout(this);
    ui_layout_->addStretch();

    ui_layout_->setAlignment(Qt::AlignCenter);
    ui_layout_->setContentsMargins(4, 4, 4, 4);
    ui_layout_->setSpacing(4);

    setFixedWidth(32);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::MinimumExpanding);

    setBackgroundRole(QPalette::Base);
    setAutoFillBackground(true);
}

} // namespace jwrite::ui
