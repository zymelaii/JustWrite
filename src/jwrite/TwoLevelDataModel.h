#pragma once

#include <QObject>

namespace jwrite {

class TwoLevelDataModel : public QObject {
    Q_OBJECT

signals:
    void valueChanged(int id, const QString &value);

public:
    explicit TwoLevelDataModel(QObject *parent = nullptr)
        : QObject(parent) {}

    virtual ~TwoLevelDataModel() = default;

    virtual int total_top_items() const                           = 0;
    virtual int total_sub_items() const                           = 0;
    virtual int total_items_under_top_item(int top_item_id) const = 0;

    virtual int top_item_at(int index)                        = 0;
    virtual int sub_item_at(int top_item_id, int index) const = 0;

    virtual QList<int> get_sub_items(int top_item_id) const = 0;

    virtual bool has_top_item(int id) const                                  = 0;
    virtual bool has_sub_item(int id) const                                  = 0;
    virtual bool has_sub_item_strict(int top_item_id, int sub_item_id) const = 0;

    virtual int add_top_item(int index, const QString &value)                  = 0;
    virtual int add_sub_item(int top_item_id, int index, const QString &value) = 0;

    virtual QString value(int id) const                     = 0;
    virtual void    set_value(int id, const QString &value) = 0;
};

} // namespace jwrite
