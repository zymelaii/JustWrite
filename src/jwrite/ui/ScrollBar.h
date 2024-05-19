#pragma once

#include <QProxyStyle>
#include <QScrollBar>

namespace jwrite::ui {

class ScrollBarProxyStyle : public QProxyStyle {
public:
    ScrollBarProxyStyle(QStyle *style = nullptr);

    QRect subControlRect(
        QStyle::ComplexControl     cc,
        const QStyleOptionComplex *opt,
        QStyle::SubControl         sc,
        const QWidget             *widget = nullptr) const override;
};

class ScrollBar : public QScrollBar {
    Q_OBJECT

public:
    ScrollBar(QWidget *parent);

protected:
    QRect sliderHandleRect() const;
    void  paintEvent(QPaintEvent *event) override;
};

} // namespace jwrite::ui
