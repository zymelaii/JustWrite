#pragma once

#include <QProxyStyle>
#include <QScrollBar>

namespace jwrite::ui {

class ScrollBarProxyStyle : public QProxyStyle {
    friend class ScrollBar;

public:
    ScrollBarProxyStyle(QStyle *style = nullptr);

    QRect subControlRect(
        QStyle::ComplexControl     cc,
        const QStyleOptionComplex *opt,
        QStyle::SubControl         sc,
        const QWidget             *widget = nullptr) const override;

private:
    int scroll_bar_edge_offset_;
};

class ScrollBar : public QScrollBar {
    Q_OBJECT

public:
    ScrollBar(QWidget *parent);

public:
    void setEdgeOffset(int offset);

protected:
    QRect sliderHandleRect() const;
    void  paintEvent(QPaintEvent *event) override;
};

} // namespace jwrite::ui
