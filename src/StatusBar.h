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

    void    setSpacing(int spacing);
    QLabel *addItem(const QString &text);
    QLabel *addItemAtRightSide(const QString &text);

private:
    StatusBarPrivate *d;
};

} // namespace jwrite
