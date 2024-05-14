#pragma once

#include <QWidget>
#include <QLabel>

namespace jwrite {

struct StatusBarPrivate;

class StatusBar : public QWidget {
    Q_OBJECT

public:
    explicit StatusBar(QWidget *parent = nullptr);
    virtual ~StatusBar();

    QSize minimumSizeHint() const override;
    QSize sizeHint() const override;

    void    set_spacing(int spacing);
    QLabel *add_item(const QString &text, bool right_side);

private:
    StatusBarPrivate *d;
};

} // namespace jwrite
