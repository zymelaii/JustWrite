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

void Toolbar::set_icon_size(int size) {
    ui_icon_size_ = size;
    setFixedWidth(ui_icon_size_ + 2 * ui_margin_);

    QList<ToolbarIcon *> toolbar_icons{};
    while (!ui_layout_->isEmpty()) {
        auto w = ui_layout_->takeAt(0)->widget();
        if (!w) { continue; }
        auto toolbar_icon = qobject_cast<ToolbarIcon *>(w);
        Q_ASSERT(toolbar_icon);
        toolbar_icon->set_icon_size(ui_icon_size_);
        toolbar_icons.append(toolbar_icon);
    }

    for (int i = 0; i < toolbar_icons.size() - ui_total_bottom_side_; ++i) {
        ui_layout_->addWidget(toolbar_icons.at(i));
    }
    ui_layout_->addStretch();
    for (int i = toolbar_icons.size() - ui_total_bottom_side_; i < toolbar_icons.size(); ++i) {
        ui_layout_->addWidget(toolbar_icons.at(i));
    }
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
    toolbar_icon->set_icon_size(ui_icon_size_);
    if (bottom_side) {
        ui_layout_->addWidget(toolbar_icon);
        ++ui_total_bottom_side_;
    } else {
        const int index = ui_layout_->count() - ui_total_bottom_side_ - 1;
        ui_layout_->insertWidget(index, toolbar_icon);
    }
}

void Toolbar::apply_mask(const QSet<int> &mask) {
    int index = 0;
    for (int i = 0; i < ui_layout_->count(); ++i) {
        auto w = ui_layout_->itemAt(i)->widget();
        if (!w) { continue; }
        w->setVisible(!mask.contains(index));
        ++index;
    }
}

Toolbar::Toolbar(QWidget *parent)
    : QWidget(parent)
    , ui_icon_size_{12}
    , ui_margin_{10}
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

    setFixedWidth(ui_icon_size_ + 2 * ui_margin_);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::MinimumExpanding);

    setBackgroundRole(QPalette::Base);
    setAutoFillBackground(true);
}

} // namespace jwrite::ui
