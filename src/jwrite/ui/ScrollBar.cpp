#include <jwrite/ui/ScrollBar.h>
#include <QStyleOptionSlider>
#include <QPainter>

namespace jwrite::ui {

ScrollBarProxyStyle::ScrollBarProxyStyle(QStyle *style)
    : QProxyStyle(style) {}

QRect ScrollBarProxyStyle::subControlRect(
    QStyle::ComplexControl     cc,
    const QStyleOptionComplex *opt,
    QStyle::SubControl         sc,
    const QWidget             *widget) const {
    if (cc != CC_ScrollBar) { return QProxyStyle::subControlRect(cc, opt, sc, widget); }

    const int bar_width = 8;
    const int bar_dx    = qMax(0, opt->rect.width() - bar_width) / 2;

    if (sc == SC_ScrollBarGroove) { return QProxyStyle::subControlRect(cc, opt, sc, widget); }

    if (sc == SC_ScrollBarSlider) {
        auto bb = QProxyStyle::subControlRect(cc, opt, sc, widget);
        bb.translate(bar_dx, 0);
        bb.setWidth(qMin(bar_width, opt->rect.width()));
        return bb;
    }

    return {};
}

ScrollBar::ScrollBar(QWidget *parent)
    : QScrollBar(Qt::Vertical, parent) {
    setStyle(new ScrollBarProxyStyle(style()));
}

void ScrollBar::paintEvent(QPaintEvent *event) {
    QPainter    p(this);
    const auto &pal = palette();
    p.setRenderHint(QPainter::Antialiasing);

    QStyleOptionSlider opt;
    initStyleOption(&opt);

    p.setPen(pal.color(QPalette::Base));
    p.fillRect(rect(), pal.base());
    p.drawRect(rect());

    const auto bb =
        style()->subControlRect(QStyle::CC_ScrollBar, &opt, QStyle::SC_ScrollBarSlider, this);

    auto color = pal.color(QPalette::Window);
    if (!opt.state.testFlag(QStyle::State_MouseOver)) { color.setAlpha(80); }

    p.setPen(color);
    p.setBrush(color);
    p.drawRoundedRect(bb, 4, 4);
}

} // namespace jwrite::ui
