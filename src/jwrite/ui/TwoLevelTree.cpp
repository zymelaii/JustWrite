#include <jwrite/ui/TwoLevelTree.h>
#include <QPaintEvent>
#include <QPainter>
#include <QMouseEvent>

namespace jwrite::ui {

TwoLevelTree::TwoLevelTree(QWidget *parent)
    : QWidget(parent)
    , model_{nullptr}
    , selected_item_{-1}
    , selected_sub_item_{-1}
    , focused_top_item_{-1}
    , render_proxy_{nullptr}
    , ui_hover_row_index_{-1} {
    setupUi();
    setMouseTracking(true);
    setItemRenderProxy(nullptr);
}

TwoLevelTree::~TwoLevelTree() {}

TwoLevelDataModel *TwoLevelTree::setModel(TwoLevelDataModel *model) {
    auto old_model = model_;
    model_         = model;

    auto update_slot = static_cast<void (QWidget::*)()>(&QWidget::update);

    if (old_model) {
        old_model->setParent(nullptr);
        disconnect(old_model, &TwoLevelDataModel::valueChanged, this, update_slot);
        disconnect(old_model, &TwoLevelDataModel::valueChanged, this, &QWidget::updateGeometry);
    }

    connect(model, &TwoLevelDataModel::valueChanged, this, update_slot);
    connect(model, &TwoLevelDataModel::valueChanged, this, &QWidget::updateGeometry);
    model->setParent(this);

    selected_item_    = -1;
    focused_top_item_ = -1;

    ellapsed_top_items_.clear();
    ui_hover_row_index_ = -1;

    update();

    return old_model;
}

void TwoLevelTree::setItemRenderProxy(ItemRenderProxy *proxy) {
    render_proxy_ = proxy;
    if (render_proxy_) {
        render_func_ = [this](QPainter *p, const QRect &clip_bb, const ItemInfo &item_info) {
            render_proxy_->render(p, clip_bb, item_info);
        };
    } else {
        render_func_ = [this](QPainter *p, const QRect &clip_bb, const ItemInfo &item_info) {
            renderItem(p, clip_bb, item_info);
        };
    }
}

int TwoLevelTree::selectedItem() const {
    return selected_item_;
}

int TwoLevelTree::selectedSubItem() const {
    return selected_sub_item_;
}

bool TwoLevelTree::setSubItemSelected(int top_item_id, int sub_item_id) {
    //! ATTENTION: even the same item id can be different (dirty), report it honestly

    if (!model_->has_sub_item_strict(top_item_id, sub_item_id)) { return false; }

    selected_sub_item_ = sub_item_id;
    selected_item_     = selected_sub_item_;
    setFocusedTopItem(top_item_id);

    emit itemSelected(false, top_item_id, sub_item_id);
    update();

    return true;
}

bool TwoLevelTree::topItemEllapsed(int top_item_id) const {
    Q_ASSERT(model_->has_top_item(top_item_id));
    return ellapsed_top_items_.contains(top_item_id);
}

void TwoLevelTree::toggleEllapsedTopItem(int top_item_id) {
    Q_ASSERT(model_->has_top_item(top_item_id));
    setTopItemEllapsed(top_item_id, !topItemEllapsed(top_item_id));
}

void TwoLevelTree::setTopItemEllapsed(int top_item_id, bool ellapse) {
    Q_ASSERT(model_->has_top_item(top_item_id));
    if (ellapse) {
        ellapsed_top_items_.insert(top_item_id);
    } else {
        ellapsed_top_items_.remove(top_item_id);
    }
    update();
}

int TwoLevelTree::focusedTopItem() const {
    return focused_top_item_;
}

void TwoLevelTree::setFocusedTopItem(int top_item_id) {
    Q_ASSERT(model_->has_top_item(top_item_id));
    if (focused_top_item_ == top_item_id) { return; }
    focused_top_item_ = top_item_id;
    update();
}

void TwoLevelTree::clearTopItemFocus() {
    if (focused_top_item_ == -1) { return; }
    focused_top_item_ = -1;
    return;
}

QSize TwoLevelTree::minimumSizeHint() const {
    const auto fm         = fontMetrics();
    const auto row_height = fm.height() + fm.descent();
    const auto height     = fm.descent() + row_height * totalVisibleItems();
    QRect      rect(0, 0, fm.averageCharWidth() * 4, height);
    return rect.marginsAdded(contentsMargins()).size();
}

QSize TwoLevelTree::sizeHint() const {
    return minimumSizeHint();
}

void TwoLevelTree::setupUi() {
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
}

QRect TwoLevelTree::getFirstRowItemRect(QRect *out_indicator_bb, int *out_sub_indent) const {
    const auto fm = fontMetrics();

    const int row_height = fm.height() + fm.descent();
    const int h_padding  = row_height;
    const int v_padding  = fm.descent();

    auto bb = contentsRect();
    bb.translate(0, v_padding);
    bb.adjust(h_padding, 0, -h_padding, 0);
    bb.setHeight(row_height);

    if (out_indicator_bb) {
        auto indicator = bb;
        indicator.translate(-h_padding, 0);
        indicator.setWidth(row_height);
        *out_indicator_bb = indicator;
    }

    if (out_sub_indent) { *out_sub_indent = 16; }

    return bb;
}

int TwoLevelTree::rowIndexAtPos(const QPoint &pos) const {
    auto bb = getFirstRowItemRect(nullptr, nullptr);

    const int row_height = bb.height();
    const int total_rows = totalVisibleItems();
    bb.setHeight(total_rows * row_height);

    if (!bb.contains(pos)) { return -1; }

    return qBound(0, (pos.y() - bb.top()) / row_height, total_rows - 1);
}

void TwoLevelTree::renderItem(QPainter *p, const QRect &clip_bb, const ItemInfo &item_info) {
    const auto flags = Qt::AlignLeft | Qt::AlignVCenter;
    p->drawText(clip_bb, flags, item_info.value);
}

void TwoLevelTree::drawIndicator(QPainter *p, const QRect &bb, const ItemInfo &item_info) {
    QString svg_name{};
    if (item_info.is_top_item) {
        const bool ellapsed = ellapsed_top_items_.contains(item_info.id);
        svg_name            = ellapsed ? "ellapse" : "expand";
    } else if (selectedSubItem() == item_info.id) {
        svg_name = "edit";
    } else {
        return;
    }

    const auto svg_file = QString(":/res/icons/indicator/%1.svg").arg(svg_name);

    const auto size  = QSize(12, 12);
    const auto delta = (bb.size() - size) / 2;

    QIcon indicator(svg_file);

    const auto pos = bb.topLeft() + QPoint(delta.width(), delta.height());

    p->drawPixmap(pos, indicator.pixmap(size));
}

int TwoLevelTree::totalVisibleItems() const {
    if (!model_) { return 0; }
    int size = totalTopItems() + totalSubItems();
    for (const int top_id : ellapsed_top_items_) { size -= totalSubItemsUnderTopItem(top_id); }
    return size;
}

void TwoLevelTree::paintEvent(QPaintEvent *event) {
    if (!model_) { return; }

    QPainter   p(this);
    const auto pal = palette();

    p.setPen(pal.color(QPalette::WindowText));
    p.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);

    int   sub_item_indent = 0;
    QRect indicator_bb{};
    auto  item_bb = getFirstRowItemRect(&indicator_bb, &sub_item_indent);

    const int row_height = item_bb.height();

    ItemInfo item_info{};
    item_info.global_index = 0;

    enum {
        Top = 0,
        Sub = 1,
    };

    int level_index[2] = {0, 0};
    int row_index      = 0;

    const auto hover_color         = pal.color(QPalette::HighlightedText);
    const auto selected_color      = pal.color(QPalette::Highlight);
    bool       should_draw_hover   = false;
    bool       found_selected_item = false;
    int        hover_item_id       = -1;
    QRect      hover_bb{};
    QRect      sel_bb{};

    const int total_top_items = totalTopItems();
    for (int i = 0; i < total_top_items; ++i) {
        const int top_id      = topItemAt(i);
        item_info.is_top_item = true;
        item_info.id          = top_id;
        item_info.local_index = 0;
        item_info.level_index = level_index[Top];
        item_info.value       = itemValue(top_id);

        drawIndicator(&p, indicator_bb, item_info);
        render_func_(&p, item_bb, item_info);

        if (row_index == ui_hover_row_index_) {
            hover_bb          = item_bb;
            hover_item_id     = top_id;
            should_draw_hover = true;
        } else if (selected_item_ == top_id) {
            sel_bb              = item_bb;
            found_selected_item = true;
        }

        ++row_index;
        item_bb.translate(0, row_height);
        indicator_bb.translate(0, row_height);

        const int total_sub_items = totalSubItemsUnderTopItem(top_id);
        if (!topItemEllapsed(top_id)) {
            item_bb.adjust(sub_item_indent, 0, 0, 0);
            indicator_bb.translate(sub_item_indent, 0);

            for (const int sub_id : getSubItems(top_id)) {
                item_info.is_top_item   = false;
                item_info.id            = sub_id;
                item_info.global_index += 1;
                item_info.local_index  += 1;
                item_info.level_index   = level_index[Sub];
                item_info.value         = itemValue(sub_id);

                drawIndicator(&p, indicator_bb, item_info);
                render_func_(&p, item_bb, item_info);

                if (row_index == ui_hover_row_index_) {
                    hover_bb          = item_bb.adjusted(-sub_item_indent, 0, 0, 0);
                    hover_item_id     = sub_id;
                    should_draw_hover = true;
                } else if (selected_item_ == sub_id) {
                    sel_bb              = item_bb.adjusted(-sub_item_indent, 0, 0, 0);
                    found_selected_item = true;
                }

                ++row_index;
                item_bb.translate(0, row_height);
                indicator_bb.translate(0, row_height);

                level_index[Sub] += 1;
            }

            item_bb.adjust(-sub_item_indent, 0, 0, 0);
            indicator_bb.translate(-sub_item_indent, 0);
        } else {
            level_index[Sub]       += total_sub_items;
            item_info.global_index += total_sub_items;
        }

        item_info.global_index += 1;
        level_index[Top]       += 1;
    }

    if (should_draw_hover) { p.fillRect(hover_bb, hover_color); }

    if (found_selected_item) {
        //! FIXME: assertion not complete
        //! HINT: case 1: selected_item_ != -1
        //! HINT: case 2: should_draw_hover && hover_item_id == selected_item_
        //! HINT: case 3: selected_item_ is an available sub item but the corresponding top item is
        //! ellapsed and the target sub item becomes a dirty value without being noticed
        p.fillRect(sel_bb, selected_color);
    }
}

void TwoLevelTree::mouseMoveEvent(QMouseEvent *e) {
    QWidget::mouseMoveEvent(e);

    const int last_row_index = ui_hover_row_index_;
    ui_hover_row_index_      = rowIndexAtPos(e->pos());
    if (ui_hover_row_index_ != last_row_index) { update(); }
}

void TwoLevelTree::leaveEvent(QEvent *e) {
    QWidget::leaveEvent(e);

    ui_hover_row_index_ = -1;
    update();
}

void TwoLevelTree::mousePressEvent(QMouseEvent *e) {
    QWidget::mousePressEvent(e);

    if (!model_) { return; }

    if (ui_hover_row_index_ == -1) {
        clearTopItemFocus();
        return;
    }
    Q_ASSERT(ui_hover_row_index_ >= 0 && ui_hover_row_index_ < totalVisibleItems());

    //! NOTE: only accept left or right button
    //! HINT: left-button: toggle ellasped top-item or select sub-item
    //! HINT: right-button: request context menu on the hover item
    if (!(e->buttons() & (Qt::LeftButton | Qt::RightButton))) { return; }

    ItemInfo item_info{};
    item_info.global_index = 0;
    item_info.id           = -1;

    enum {
        Top = 0,
        Sub = 1,
    };

    int level_index[2] = {0, 0};
    int item_id[2]     = {-1, -1};
    int row_index      = 0;

    const int total_top_items = totalTopItems();
    for (int i = 0; i < total_top_items; ++i) {
        const int top_id = topItemAt(i);
        item_id[Top]     = top_id;

        if (row_index == ui_hover_row_index_) {
            item_info.is_top_item = true;
            item_info.id          = top_id;
            item_info.local_index = 0;
            item_info.level_index = level_index[Top];
            break;
        }

        ++item_info.global_index;

        const int total_sub_items = totalSubItemsUnderTopItem(top_id);
        if (!topItemEllapsed(top_id)) {
            int local_index = 0;
            for (const int sub_id : getSubItems(top_id)) {
                ++row_index;
                if (row_index == ui_hover_row_index_) {
                    item_id[Sub]            = sub_id;
                    item_info.global_index += local_index;
                    level_index[Sub]       += local_index;
                    item_info.is_top_item   = false;
                    item_info.id            = sub_id;
                    item_info.local_index   = local_index;
                    item_info.level_index   = level_index[Sub];
                    break;
                }
                ++local_index;
            }
            if (item_info.id != -1) { break; }
        }

        item_info.global_index += total_sub_items;
        level_index[Top]       += 1;
        level_index[Sub]       += total_sub_items;
        ++row_index;
    }

    Q_ASSERT(item_info.id != -1);

    if (e->button() == Qt::LeftButton) {
        if (item_info.is_top_item) {
            toggleEllapsedTopItem(item_info.id);
            selected_item_ = item_id[Top];
        } else {
            Q_ASSERT(item_id[Sub] == item_info.id);
            selected_item_     = item_id[Sub];
            selected_sub_item_ = selected_item_;
        }
        emit itemSelected(item_info.is_top_item, item_id[Top], item_id[Sub]);
        setFocusedTopItem(item_id[Top]);
        update();
    } else if (e->button() == Qt::RightButton) {
        emit contextMenuRequested(e->pos(), item_info);
    }
}

} // namespace jwrite::ui
