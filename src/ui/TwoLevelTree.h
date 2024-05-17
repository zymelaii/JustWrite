#pragma once

#include <QWidget>

namespace jwrite::Ui {

class TwoLevelTree : public QWidget {
    Q_OBJECT

public:
    struct ItemInfo {
        bool    is_top_item;
        int     id;
        int     global_index;
        int     local_index;
        int     level_index;
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
        return top_item_id_list_.size();
    }

    int topItemAt(int index) const {
        return top_item_id_list_.value(index, -1);
    }

    int totalSubItemsUnderTopItem(int top_item_id) const;
    int totalSubItems() const;

    QList<int> getSubItems(int top_item_id) const {
        return sub_item_id_list_set_.value(top_item_id, {});
    }

    int getSubItem(int top_item_id, int index) const {
        if (!sub_item_id_list_set_.contains(top_item_id)) { return -1; }
        return sub_item_id_list_set_[top_item_id].value(index, -1);
    }

    int addTopItem(int index, const QString &value);
    int addSubItem(int top_item_id, int index, const QString &value);

    QString itemValue(int id) const;
    void    setItemValue(int id, const QString &value);

    bool setSubItemSelected(int top_item_id, int sub_item_id);

    void setItemRenderProxy(ItemRenderProxy *proxy);

public:
    QSize minimumSizeHint() const override;
    QSize sizeHint() const override;

protected:
    void setupUi();
    void renderItem(QPainter *p, const QRect &clip_bb, const ItemInfo &item_info);

    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private:
    QList<int>            top_item_id_list_;
    QMap<int, QList<int>> sub_item_id_list_set_;
    QList<QString>        item_values_;
    QList<QString>        title_list_;
    QSet<int>             expanded_top_items_;
    int                   selected_sub_item_;
    ItemRenderProxy      *render_proxy_;

    std::function<void(QPainter *, const QRect &, const ItemInfo &)> render_func_;

    int ui_hover_on_;
};

} // namespace jwrite::Ui
