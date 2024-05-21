#pragma once

#include <jwrite/TwoLevelDataModel.h>
#include <QWidget>

namespace jwrite::ui {

class TwoLevelTree : public QWidget {
    Q_OBJECT

public:
    struct ItemInfo {
        //! whether the item is a top-level item or a sub-item
        bool is_top_item;
        //! unique id of the item
        int id;
        //! global index of the item in the tree
        int global_index;
        //! index of the item in its level, sorted with nearby same-level items
        int local_index;
        //! index of the item in its level, sorted in the global order of all items
        int level_index;
        //! value of the item
        QString value;
    };

    class ItemRenderProxy {
    public:
        virtual void render(QPainter *p, const QRect &clip_bb, const ItemInfo &item_info) = 0;
    };

public:
    explicit TwoLevelTree(QWidget *parent = nullptr);
    virtual ~TwoLevelTree();

signals:
    void subItemSelected(int top_item_id, int sub_item_id);
    void contextMenuRequested(QPoint pos, ItemInfo item_info);

public:
    int totalTopItems() const {
        return model_->total_top_items();
    }

    int topItemAt(int index) const {
        return model_->top_item_at(index);
    }

    int totalSubItemsUnderTopItem(int top_item_id) const {
        return model_->total_items_under_top_item(top_item_id);
    }

    int totalSubItems() const {
        return model_->total_sub_items();
    }

    QList<int> getSubItems(int top_item_id) const {
        return model_->get_sub_items(top_item_id);
    }

    int getSubItem(int top_item_id, int index) const {
        return model_->sub_item_at(top_item_id, index);
    }

    int addTopItem(int index, const QString &value) {
        return model_->add_top_item(index, value);
    }

    int addSubItem(int top_item_id, int index, const QString &value) {
        return model_->add_sub_item(top_item_id, index, value);
    }

    QString itemValue(int id) const {
        return model_->value(id);
    }

    void setItemValue(int id, const QString &value) {
        model_->set_value(id, value);
    }

    TwoLevelDataModel *model() {
        return model_;
    }

    TwoLevelDataModel *model() const {
        return const_cast<TwoLevelTree *>(this)->model_;
    }

    TwoLevelDataModel *setModel(TwoLevelDataModel *model);

    bool setSubItemSelected(int top_item_id, int sub_item_id);

    void setItemRenderProxy(ItemRenderProxy *proxy);

    bool topItemEllapsed(int top_item_id) const;
    void toggleEllapsedTopItem(int top_item_id);
    void setTopItemEllapsed(int top_item_id, bool ellapse);
    int  focuedTopItem() const;
    void setFocusedTopItem(int top_item_id);
    void clearTopItemFocus();

public:
    QSize minimumSizeHint() const override;
    QSize sizeHint() const override;

protected:
    void setupUi();

    QRect getFirstRowItemRect(QRect *out_indicator_bb, int *out_sub_indent) const;
    int   rowIndexAtPos(const QPoint &pos) const;

    void renderItem(QPainter *p, const QRect &clip_bb, const ItemInfo &item_info);
    void drawIndicator(QPainter *p, const QRect &bb, const ItemInfo &item_info);
    int  totalVisibleItems() const;

    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private:
    TwoLevelDataModel *model_;
    QSet<int>          ellapsed_top_items_;
    int                selected_sub_item_;
    int                focused_top_item_;
    ItemRenderProxy   *render_proxy_;

    std::function<void(QPainter *, const QRect &, const ItemInfo &)> render_func_;

    int ui_hover_row_index_;
};

} // namespace jwrite::ui
