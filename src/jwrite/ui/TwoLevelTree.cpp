#include <jwrite/ui/TwoLevelTree.h>
#include <QPaintEvent>
#include <QPainter>
#include <QMouseEvent>

namespace jwrite::ui {

TwoLevelTree::TwoLevelTree(QWidget *parent)
    : QWidget(parent)
    , model_{nullptr}
    , selected_sub_item_{-1}
    , render_proxy_{nullptr}
    , ui_hover_on_{-1} {
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
    }

    connect(model, &TwoLevelDataModel::valueChanged, this, update_slot);
    model->setParent(this);

    ellapsed_top_items_.clear();
    ui_hover_on_ = -1;

    update();

    return old_model;
}

bool TwoLevelTree::setSubItemSelected(int top_item_id, int sub_item_id) {
    //! ATTENTION: even the same item id can be different (dirty), report it honestly

    if (!model_->has_sub_item_strict(top_item_id, sub_item_id)) { return false; }

    selected_sub_item_ = sub_item_id;
    emit subItemSelected(top_item_id, sub_item_id);
    update();

    return true;
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

QSize TwoLevelTree::minimumSizeHint() const {
    const auto fm         = fontMetrics();
    const auto row_height = fm.height() + fm.descent();
    QRect      rect(0, 0, fm.averageCharWidth() * 4, fm.descent() + row_height);
    return rect.marginsAdded(contentsMargins()).size();
}

QSize TwoLevelTree::sizeHint() const {
    const auto fm         = fontMetrics();
    const auto row_height = fm.height() + fm.descent();
    const auto height     = fm.descent() + row_height * totalVisibleItems();
    QRect      rect(0, 0, fm.averageCharWidth() * 4, height);
    return rect.marginsAdded(contentsMargins()).size();
}

void TwoLevelTree::setupUi() {
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
}

void TwoLevelTree::renderItem(QPainter *p, const QRect &clip_bb, const ItemInfo &item_info) {
    const auto flags = Qt::AlignLeft | Qt::AlignVCenter;
    p->drawText(clip_bb, flags, item_info.value);
}

int TwoLevelTree::totalVisibleItems() const {
    if (!model_) { return 0; }
    int size = model_->total_top_items() + model_->total_sub_items();
    for (auto topid : ellapsed_top_items_) { size -= model_->total_items_under_top_item(topid); }
    return size;
}

void TwoLevelTree::paintEvent(QPaintEvent *event) {
    if (!model_) { return; }

    QPainter   p(this);
    const auto pal = palette();

    p.setPen(pal.color(QPalette::WindowText));
    p.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);

    const auto fm         = fontMetrics();
    const int  row_height = fm.height() + fm.descent();

    auto clip_bb = contentsRect();
    clip_bb.adjust(row_height, 0, -row_height, 0);
    clip_bb.translate(0, fm.descent());
    clip_bb.setHeight(row_height);

    const auto hover_color     = pal.color(QPalette::HighlightedText);
    const auto selected_color  = pal.color(QPalette::Highlight);
    const int  sub_item_indent = 16;

    ItemInfo item_info{};
    item_info.global_index = 0;

    int level_index[2] = {0, 0};

    QRect hover_item_bb{};
    QRect selected_item_bb{};
    bool  should_draw_hover_highlight = false;

    //! TODO: handle ellapsed top items

    const int total_top_items = model_->total_top_items();
    for (int i = 0; i < total_top_items; ++i) {
        const int top_id      = model_->top_item_at(i);
        item_info.is_top_item = true;
        item_info.id          = top_id;
        item_info.value       = model_->value(top_id);
        item_info.level_index = level_index[0];
        render_func_(&p, clip_bb, item_info);
        clip_bb.translate(0, row_height);
        clip_bb.adjust(sub_item_indent, 0, 0, 0);
        ++level_index[0];
        ++item_info.global_index;
        item_info.local_index = 0;
        item_info.is_top_item = false;
        for (auto sub_id : model_->get_sub_items(top_id)) {
            item_info.id          = sub_id;
            item_info.value       = model_->value(sub_id);
            item_info.level_index = level_index[1];
            render_func_(&p, clip_bb, item_info);
            if (!should_draw_hover_highlight && ui_hover_on_ == item_info.global_index) {
                should_draw_hover_highlight = true;
                hover_item_bb               = clip_bb.adjusted(-sub_item_indent, 0, 0, 0);
            } else if (selected_sub_item_ == item_info.id) {
                selected_item_bb = clip_bb.adjusted(-sub_item_indent, 0, 0, 0);
            }
            clip_bb.translate(0, row_height);
            ++level_index[1];
            ++item_info.global_index;
            ++item_info.local_index;
        }
        clip_bb.adjust(-sub_item_indent, 0, 0, 0);
    }

    if (should_draw_hover_highlight) { p.fillRect(hover_item_bb, hover_color); }

    if (selected_sub_item_ != -1) { p.fillRect(selected_item_bb, selected_color); }
}

void TwoLevelTree::mouseMoveEvent(QMouseEvent *e) {
    QWidget::mouseMoveEvent(e);

    const auto fm         = fontMetrics();
    const auto row_height = fm.height() + fm.descent();
    auto       rect       = contentsRect();
    rect.adjust(row_height, fm.descent(), 0, 0);

    const int last_hover_row = ui_hover_on_;

    if (rect.contains(e->pos())) {
        ui_hover_on_ = (e->pos().y() - rect.top()) / row_height;
    } else {
        ui_hover_on_ = -1;
    }

    if (ui_hover_on_ != last_hover_row) { update(); }
}

void TwoLevelTree::leaveEvent(QEvent *e) {
    QWidget::leaveEvent(e);

    if (ui_hover_on_ != -1) {
        ui_hover_on_ = -1;
        update();
    }
}

void TwoLevelTree::mousePressEvent(QMouseEvent *e) {
    QWidget::mousePressEvent(e);

    if (!model_) { return; }

    if (e->button() == Qt::MiddleButton) { return; }
    if (ui_hover_on_ == -1) { return; }
    if (ui_hover_on_ >= totalVisibleItems()) { return; }

    ItemInfo item_info{};
    item_info.global_index = 0;

    int top_item_id = -1;
    int sub_item_id = -1;

    int level_index[2] = {0, 0};

    const int total_top_items = model_->total_top_items();
    for (int i = 0; i < total_top_items; ++i) {
        const int top_id = model_->top_item_at(i);

        if (item_info.global_index == ui_hover_on_) {
            item_info.is_top_item = true;
            item_info.id          = top_id;
            item_info.level_index = level_index[0];
            top_item_id           = top_id;
            break;
        }

        ++level_index[0];
        ++item_info.global_index;
        item_info.local_index = 0;

        for (auto sub_id : model_->get_sub_items(top_id)) {
            if (item_info.global_index == ui_hover_on_) {
                item_info.is_top_item = false;
                item_info.id          = sub_id;
                item_info.level_index = level_index[1];
                top_item_id           = top_id;
                sub_item_id           = sub_id;
                break;
            }

            ++level_index[1];
            ++item_info.global_index;
            ++item_info.local_index;
        }
    }

    if (e->button() == Qt::LeftButton) {
        if (sub_item_id != -1) {
            selected_sub_item_ = sub_item_id;
            emit subItemSelected(top_item_id, sub_item_id);
        }
    } else if (e->button() == Qt::RightButton) {
        emit contextMenuRequested(e->pos(), item_info);
    }
}

} // namespace jwrite::ui
