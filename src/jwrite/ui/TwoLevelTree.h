#pragma once

#include <jwrite/TwoLevelDataModel.h>
#include <QWidget>
#include <QTimer>
#include <memory>
#include <functional>

namespace jwrite::ui {

struct TwoLevelTreeItemInfo {
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

class TwoLevelTreeItemRenderProxy {
public:
    using ItemInfo = TwoLevelTreeItemInfo;

public:
    virtual ~TwoLevelTreeItemRenderProxy() = default;

    virtual void render(QPainter *p, const QRect &clip_bb, const ItemInfo &item_info) = 0;
};

class DefaultTwoLevelTreeItemRender : public TwoLevelTreeItemRenderProxy {
public:
    virtual void render(QPainter *p, const QRect &clip_bb, const ItemInfo &item_info) override;
};

class TwoLevelTree : public QWidget {
    Q_OBJECT

public:
    explicit TwoLevelTree(QWidget *parent = nullptr);

    ~TwoLevelTree() override;

public:
    using ItemInfo        = TwoLevelTreeItemInfo;
    using ItemRenderProxy = TwoLevelTreeItemRenderProxy;

    enum class Indicator {
        Edit,
        Ellapse,
        Expand,
    };

signals:
    void itemSelected(bool is_top_item, int top_item_id, int sub_item_id);
    void itemDoubleClicked(bool is_top_item, int top_item_id, int sub_item_id);
    void contextMenuRequested(QPoint pos, ItemInfo item_info);

public:
    int totalTopItems() const {
        Q_ASSERT(model_);
        return model_->total_top_items();
    }

    int topItemAt(int index) const {
        Q_ASSERT(model_);
        return model_->top_item_at(index);
    }

    int totalSubItemsUnderTopItem(int top_item_id) const {
        Q_ASSERT(model_);
        return model_->total_items_under_top_item(top_item_id);
    }

    int totalSubItems() const {
        Q_ASSERT(model_);
        return model_->total_sub_items();
    }

    QList<int> getSubItems(int top_item_id) const {
        Q_ASSERT(model_);
        return model_->get_sub_items(top_item_id);
    }

    int getSubItem(int top_item_id, int index) const {
        Q_ASSERT(model_);
        return model_->sub_item_at(top_item_id, index);
    }

    int addTopItem(int index, const QString &value) {
        Q_ASSERT(model_);
        return model_->add_top_item(index, value);
    }

    int addSubItem(int top_item_id, int index, const QString &value) {
        Q_ASSERT(model_);
        return model_->add_sub_item(top_item_id, index, value);
    }

    QString itemValue(int id) const {
        Q_ASSERT(model_);
        return model_->value(id);
    }

    void setItemValue(int id, const QString &value) {
        Q_ASSERT(model_);
        model_->set_value(id, value);
    }

    TwoLevelDataModel *model() const {
        Q_ASSERT(model_);
        return model_.get();
    }

    void resetState();

    void setModel(std::unique_ptr<TwoLevelDataModel> model);

    void setItemRenderProxy(std::unique_ptr<ItemRenderProxy> proxy);

    int  selectedItem() const;
    int  selectedSubItem() const;
    bool setSubItemSelected(int top_item_id, int sub_item_id);
    bool topItemEllapsed(int top_item_id) const;
    void toggleEllapsedTopItem(int top_item_id);
    void setTopItemEllapsed(int top_item_id, bool ellapse);
    int  focusedTopItem() const;
    void setFocusedTopItem(int top_item_id);
    void clearTopItemFocus();

    int doubleClickInterval() const {
        return double_click_interval_;
    }

    void setDoubleClickInterval(int interval_ms) {
        double_click_interval_ = qBound(50, interval_ms, 3000);
    }

    void reloadIndicator();

    QSize indicatorSize() const {
        return QSize(12, 12);
    }

public:
    QSize minimumSizeHint() const override;
    QSize sizeHint() const override;

protected:
    void setupUi();

    QRect getFirstRowItemRect(QRect *out_indicator_bb, int *out_sub_indent) const;
    int   rowIndexAtPos(const QPoint &pos, bool *out_is_indicator) const;

    void renderItem(QPainter *p, const QRect &clip_bb, const ItemInfo &item_info);
    void drawIndicator(QPainter *p, const QRect &bb, const ItemInfo &item_info);
    int  totalVisibleItems() const;

    void handleButtonClick(const QPoint &pos, Qt::MouseButton button, bool test_single_click);
    void handleSingleClick(const ItemInfo &item_info, int top_item_id, int sub_item_id);
    void cancelSingleClickEvent();
    void prepareSingleClickEvent(std::function<void()> action);

    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;

private:
    struct ClickedItemInfo {
        ItemInfo item_info;
        int      top_item_id;
        int      sub_item_id;
    };

    std::unique_ptr<TwoLevelDataModel> model_;
    std::unique_ptr<ItemRenderProxy>   item_render_proxy_;

    QSet<int> ellapsed_top_items_;
    int       selected_item_;
    int       selected_sub_item_;
    int       focused_top_item_;

    ClickedItemInfo clicked_item_info_;
    QTimer          single_click_timer_;
    int             double_click_interval_;

    int                      ui_hover_row_index_;
    bool                     ui_hover_on_indicator_;
    QMap<Indicator, QPixmap> ui_indicators_;
};

QDebug operator<<(QDebug stream, const TwoLevelTreeItemInfo &item_info);

} // namespace jwrite::ui
