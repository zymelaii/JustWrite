#include <jwrite/ui/TwoLevelTree.h>
#include <jwrite/AppConfig.h>
#include <QPaintEvent>
#include <QPainter>
#include <QMouseEvent>
#include <magic_enum.hpp>

namespace jwrite::ui {

void DefaultTwoLevelTreeItemRender::render(
    QPainter *p, const QRect &clip_bb, const ItemInfo &item_info) {
    const auto flags = Qt::AlignLeft | Qt::AlignVCenter;
    p->drawText(clip_bb, flags, item_info.value);
}

TwoLevelTree::TwoLevelTree(QWidget *parent)
    : QWidget(parent)
    , single_click_timer_(this)
    , double_click_interval_{200} {
    setupUi();
    setMouseTracking(true);

    resetState();
    setItemRenderProxy(std::make_unique<DefaultTwoLevelTreeItemRender>());
}

TwoLevelTree::~TwoLevelTree() {}

void TwoLevelTree::resetState() {
    selected_item_         = -1;
    selected_sub_item_     = -1;
    focused_top_item_      = -1;
    ui_hover_row_index_    = -1;
    ui_hover_on_indicator_ = false;
    ellapsed_top_items_.clear();
    update();
}

void TwoLevelTree::setModel(std::unique_ptr<TwoLevelDataModel> model) {
    Q_ASSERT(model);

    model_ = std::move(model);
    model_->setParent(this);

    connect(
        model_.get(),
        &TwoLevelDataModel::valueChanged,
        this,
        static_cast<void (QWidget::*)()>(&QWidget::update));
    connect(model_.get(), &TwoLevelDataModel::valueChanged, this, &QWidget::updateGeometry);

    resetState();

    adjustSize();
}

void TwoLevelTree::setItemRenderProxy(std::unique_ptr<ItemRenderProxy> proxy) {
    Q_ASSERT(proxy);
    item_render_proxy_ = std::move(proxy);
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

void TwoLevelTree::reloadIndicator() {
    auto      &config = AppConfig::get_instance();
    const auto size   = indicatorSize();
    for (const auto type : magic_enum::enum_values<Indicator>()) {
        const auto name      = magic_enum::enum_name(type);
        const auto icon_path = config.icon(QString("indicator/%1").arg(name.data()).toLower());
        QIcon      icon(icon_path);
        Q_ASSERT(!icon.isNull());
        ui_indicators_[type] = icon.pixmap(size);
    }
}

QSize TwoLevelTree::minimumSizeHint() const {
    QRect      indicator_bb{};
    auto       bb     = getFirstRowItemRect(&indicator_bb, nullptr);
    const auto fm     = fontMetrics();
    const int  height = bb.bottom() + bb.height() * totalVisibleItems();
    QRect      rect(0, 0, fm.averageCharWidth() * 4, height);
    return rect.marginsAdded(contentsMargins()).size();
}

QSize TwoLevelTree::sizeHint() const {
    return minimumSizeHint();
}

void TwoLevelTree::setupUi() {
    reloadIndicator();
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

    if (out_sub_indent) { *out_sub_indent = 12; }

    return bb;
}

int TwoLevelTree::rowIndexAtPos(const QPoint &pos, bool *out_is_indicator) const {
    QRect indicator_bb{};
    auto  bb = getFirstRowItemRect(&indicator_bb, nullptr);

    //! FIXME: validity check
    bb.setTopLeft(indicator_bb.topLeft());

    const int row_height = bb.height();
    const int total_rows = totalVisibleItems();
    bb.setHeight(total_rows * row_height);

    if (!bb.contains(pos)) { return -1; }

    const int row = qBound(0, (pos.y() - bb.top()) / row_height, total_rows - 1);

    if (out_is_indicator) { *out_is_indicator = indicator_bb.contains(pos); }

    return row;
}

void TwoLevelTree::drawIndicator(QPainter *p, const QRect &bb, const ItemInfo &item_info) {
    if (!item_info.is_top_item && selectedSubItem() != item_info.id) { return; }
    const auto type = !item_info.is_top_item                     ? Indicator::Edit
                    : ellapsed_top_items_.contains(item_info.id) ? Indicator::Ellapse
                                                                 : Indicator::Expand;
    Q_ASSERT(ui_indicators_.contains(type));
    const auto &indicator = ui_indicators_[type];
    const auto  delta     = (bb.size() - indicatorSize()) / 2;
    const auto  pos       = bb.topLeft() + QPoint(delta.width(), delta.height());
    p->drawPixmap(pos, indicator);
}

int TwoLevelTree::totalVisibleItems() const {
    if (!model_) { return 0; }
    int size = totalTopItems() + totalSubItems();
    for (const int top_id : ellapsed_top_items_) { size -= totalSubItemsUnderTopItem(top_id); }
    return size;
}

void TwoLevelTree::handleButtonClick(
    const QPoint &pos, Qt::MouseButton button, bool test_single_click) {
    if (!model_) { return; }

    if (ui_hover_row_index_ == -1) {
        clearTopItemFocus();
        return;
    }
    Q_ASSERT(ui_hover_row_index_ >= 0 && ui_hover_row_index_ < totalVisibleItems());

    //! NOTE: only accept left or right button
    //! HINT: left-button: toggle ellasped top-item or select sub-item
    //! HINT: right-button: request context menu on the hover item
    if (button != Qt::LeftButton && button != Qt::RightButton) { return; }

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

    if (button == Qt::LeftButton) {
        QRect indicator_bb{};
        getFirstRowItemRect(&indicator_bb, nullptr);
        indicator_bb.translate(0, ui_hover_row_index_ * indicator_bb.height());
        const bool indicator_clicked = indicator_bb.contains(pos);
        if (indicator_clicked && item_info.is_top_item) {
            toggleEllapsedTopItem(item_info.id);
            update();
        } else if (test_single_click) {
            handleSingleClick(item_info, item_id[Top], item_id[Sub]);
        }
    } else if (button == Qt::RightButton) {
        emit contextMenuRequested(pos, item_info);
    }
}

void TwoLevelTree::handleSingleClick(const ItemInfo &item_info, int top_item_id, int sub_item_id) {
    cancelSingleClickEvent();

    clicked_item_info_.item_info   = item_info;
    clicked_item_info_.top_item_id = top_item_id;
    clicked_item_info_.sub_item_id = sub_item_id;

    prepareSingleClickEvent([this] {
        if (clicked_item_info_.item_info.is_top_item) {
            selected_item_ = clicked_item_info_.top_item_id;
        } else {
            Q_ASSERT(clicked_item_info_.sub_item_id == clicked_item_info_.item_info.id);
            selected_item_     = clicked_item_info_.sub_item_id;
            selected_sub_item_ = selected_item_;
        }

        emit itemSelected(
            clicked_item_info_.item_info.is_top_item,
            clicked_item_info_.top_item_id,
            clicked_item_info_.sub_item_id);
        setFocusedTopItem(clicked_item_info_.top_item_id);

        update();

        disconnect(&single_click_timer_, &QTimer::timeout, this, nullptr);
    });
}

void TwoLevelTree::cancelSingleClickEvent() {
    single_click_timer_.stop();
    disconnect(&single_click_timer_, &QTimer::timeout, this, nullptr);
}

void TwoLevelTree::prepareSingleClickEvent(std::function<void()> action) {
    Q_ASSERT(!single_click_timer_.isActive());

    //! NOTE: QApplication::doubleClickInterval() seems too long to wait a double click

    single_click_timer_.setSingleShot(true);
    single_click_timer_.setInterval(double_click_interval_);
    connect(&single_click_timer_, &QTimer::timeout, this, action);

    single_click_timer_.start();
}

void TwoLevelTree::paintEvent(QPaintEvent *event) {
    if (!model_) { return; }
    if (!item_render_proxy_) { return; }

    QPainter   p(this);
    const auto pal = palette();

    const auto clip_bb    = event->rect();
    bool       clip_begin = false;

    p.setPen(pal.color(QPalette::WindowText));
    p.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);

    int       sub_item_indent = 0;
    QRect     indicator_bb{};
    auto      item_bb = getFirstRowItemRect(&indicator_bb, &sub_item_indent);
    const int h_slack = 4;

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

        if (clip_bb.intersects(item_bb)) {
            drawIndicator(&p, indicator_bb, item_info);
            item_render_proxy_->render(&p, item_bb.adjusted(h_slack, 0, -h_slack, 0), item_info);

            //! NOTE: indicator can be in the same row with the hover item and the selected item
            if (row_index == ui_hover_row_index_) {
                hover_bb          = item_bb;
                hover_item_id     = top_id;
                should_draw_hover = true;
            }
            if (selected_item_ == top_id) {
                sel_bb              = item_bb;
                found_selected_item = true;
            }
            clip_begin = true;
        } else if (clip_begin) {
            break;
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

                if (clip_bb.intersects(item_bb)) {
                    drawIndicator(&p, indicator_bb.translated(h_slack, 0), item_info);
                    item_render_proxy_->render(
                        &p, item_bb.adjusted(h_slack, 0, -h_slack, 0), item_info);
                    if (row_index == ui_hover_row_index_) {
                        hover_bb          = item_bb.adjusted(-sub_item_indent, 0, 0, 0);
                        hover_item_id     = sub_id;
                        should_draw_hover = true;
                    }
                    if (selected_item_ == sub_id) {
                        sel_bb              = item_bb.adjusted(-sub_item_indent, 0, 0, 0);
                        found_selected_item = true;
                    }
                    clip_begin = true;
                } else if (clip_begin) {
                    break;
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

    should_draw_hover = should_draw_hover && !ui_hover_on_indicator_;
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

void TwoLevelTree::mouseMoveEvent(QMouseEvent *event) {
    const QPair<int, bool> old_hover_hint{ui_hover_row_index_, ui_hover_on_indicator_};
    ui_hover_row_index_ = rowIndexAtPos(event->pos(), &ui_hover_on_indicator_);
    const QPair<int, bool> hover_hint{ui_hover_row_index_, ui_hover_on_indicator_};

    if (hover_hint != old_hover_hint) { update(); }
}

void TwoLevelTree::leaveEvent(QEvent *event) {
    ui_hover_row_index_ = -1;
    update();
}

void TwoLevelTree::mousePressEvent(QMouseEvent *event) {
    handleButtonClick(event->pos(), event->button(), true);
}

void TwoLevelTree::mouseDoubleClickEvent(QMouseEvent *event) {
    if (event->button() != Qt::LeftButton) { return; }

    if (single_click_timer_.isActive()) {
        cancelSingleClickEvent();
        emit itemDoubleClicked(
            clicked_item_info_.item_info.is_top_item,
            clicked_item_info_.top_item_id,
            clicked_item_info_.sub_item_id);
    } else {
        handleButtonClick(event->pos(), event->button(), false);
    }
}

QDebug operator<<(QDebug stream, const TwoLevelTreeItemInfo &item_info) {
    stream.nospace() << "ItemInfo::" << (item_info.is_top_item ? "Top" : "Sub")
                     << "(id=" << item_info.id << ", index=[" << item_info.global_index << ","
                     << item_info.local_index << "," << item_info.level_index
                     << "], value=" << item_info.value << ")";
    return stream;
}

} // namespace jwrite::ui
