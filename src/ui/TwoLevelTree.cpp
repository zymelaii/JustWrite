#include "TwoLevelTree.h"
#include <QPaintEvent>
#include <QPainter>
#include <QMouseEvent>

namespace jwrite::Ui {

TwoLevelTree::TwoLevelTree(QWidget *parent)
    : QWidget(parent)
    , ui_hover_on_{-1} {
    setupUi();
    setMouseTracking(true);
    setItemRenderProxy(nullptr);
}

TwoLevelTree::~TwoLevelTree() {}

int TwoLevelTree::totalSubItemsUnderTopItem(int top_item_id) const {
    if (!sub_item_id_list_set_.contains(top_item_id)) { return 0; }
    return sub_item_id_list_set_[top_item_id].size();
}

int TwoLevelTree::totalSubItems() const {
    return title_list_.size() - top_item_id_list_.size();
}

int TwoLevelTree::addTopItem(int index, const QString &value) {
    if (index < 0 || index > static_cast<int>(top_item_id_list_.size())) { return -1; }
    const int id = static_cast<int>(title_list_.size());
    title_list_.push_back(value);
    top_item_id_list_.insert(index, id);
    sub_item_id_list_set_.insert(id, {});
    update();
    return id;
}

int TwoLevelTree::addSubItem(int top_item_id, int index, const QString &value) {
    if (!sub_item_id_list_set_.contains(top_item_id)) { return -1; }
    auto &sub_item_id_list = sub_item_id_list_set_[top_item_id];
    if (index < 0 || index > static_cast<int>(sub_item_id_list.size())) { return -1; }
    const int id = static_cast<int>(title_list_.size());
    title_list_.push_back(value);
    sub_item_id_list.insert(index, id);
    update();
    return id;
}

QString TwoLevelTree::itemValue(int id) const {
    Q_ASSERT(id >= 0 && id < title_list_.size());
    return title_list_[id];
}

void TwoLevelTree::setItemValue(int id, const QString &value) {
    Q_ASSERT(id >= 0 && id < title_list_.size());
    title_list_[id] = value;
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
    const auto height     = fm.descent() + row_height * title_list_.size();
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

void TwoLevelTree::paintEvent(QPaintEvent *event) {
    QPainter   p(this);
    const auto pal = palette();

    p.setPen(pal.color(QPalette::Text));
    p.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);

    const auto fm         = fontMetrics();
    const int  row_height = fm.height() + fm.descent();

    auto clip_bb = contentsRect();
    clip_bb.adjust(row_height, 0, -row_height, 0);
    clip_bb.translate(0, fm.descent());
    clip_bb.setHeight(row_height);

    const auto hover_color     = pal.color(QPalette::Highlight);
    const int  sub_item_indent = 16;

    ItemInfo item_info{};
    item_info.global_index = 0;

    int level_index[2] = {0, 0};

    QRect hover_item_bb{};
    bool  should_draw_hover_highlight = false;

    for (auto topid : top_item_id_list_) {
        item_info.is_top_item = true;
        item_info.id          = topid;
        item_info.value       = title_list_[item_info.id];
        item_info.level_index = level_index[0];
        render_func_(&p, clip_bb, item_info);
        clip_bb.translate(0, row_height);
        clip_bb.adjust(sub_item_indent, 0, 0, 0);
        ++level_index[0];
        ++item_info.global_index;
        item_info.local_index = 0;
        item_info.is_top_item = false;
        for (auto subid : sub_item_id_list_set_[topid]) {
            item_info.id          = subid;
            item_info.value       = title_list_[item_info.id];
            item_info.level_index = level_index[1];
            render_func_(&p, clip_bb, item_info);
            if (!should_draw_hover_highlight && ui_hover_on_ == item_info.global_index) {
                should_draw_hover_highlight = true;
                hover_item_bb               = clip_bb.adjusted(-sub_item_indent, 0, 0, 0);
            }
            clip_bb.translate(0, row_height);
            ++level_index[1];
            ++item_info.global_index;
            ++item_info.local_index;
        }
        clip_bb.adjust(-sub_item_indent, 0, 0, 0);
    }

    if (should_draw_hover_highlight) { p.fillRect(hover_item_bb, hover_color); }
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

    if (e->button() == Qt::MiddleButton) { return; }
    if (ui_hover_on_ == -1) { return; }
    if (ui_hover_on_ >= static_cast<int>(title_list_.size())) { return; }

    ItemInfo item_info{};
    item_info.global_index = 0;

    int top_item_id = -1;
    int sub_item_id = -1;

    int level_index[2] = {0, 0};

    for (auto topid : top_item_id_list_) {
        if (item_info.global_index == ui_hover_on_) {
            item_info.is_top_item = true;
            item_info.id          = topid;
            item_info.level_index = level_index[0];
            top_item_id           = topid;
            break;
        }
        ++level_index[0];
        ++item_info.global_index;
        item_info.local_index = 0;
        for (auto subid : sub_item_id_list_set_[topid]) {
            if (item_info.global_index == ui_hover_on_) {
                item_info.is_top_item = false;
                item_info.id          = subid;
                item_info.level_index = level_index[1];
                top_item_id           = topid;
                sub_item_id           = subid;
                break;
            }
            ++level_index[1];
            ++item_info.global_index;
            ++item_info.local_index;
        }
    }

    if (e->button() == Qt::LeftButton) {
        if (sub_item_id != -1) { emit subItemSelected(top_item_id, sub_item_id); }
    } else if (e->button() == Qt::RightButton) {
        emit contextMenuRequested(e->pos(), item_info);
    }
}

} // namespace jwrite::Ui
