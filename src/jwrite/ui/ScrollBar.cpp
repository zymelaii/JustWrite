#include <jwrite/ui/ScrollBar.h>
#include <QStyleOptionSlider>
#include <QPainter>

namespace jwrite::ui {

ScrollBarProxyStyle::ScrollBarProxyStyle(QStyle *style)
    : QProxyStyle(style)
    , scroll_bar_edge_offset_{0} {}

QRect ScrollBarProxyStyle::subControlRect(
    QStyle::ComplexControl     cc,
    const QStyleOptionComplex *opt,
    QStyle::SubControl         sc,
    const QWidget             *widget) const {
    if (cc != CC_ScrollBar) { return QProxyStyle::subControlRect(cc, opt, sc, widget); }

    const int bar_width = 8;
    const int bar_dx    = qMax(0, opt->rect.width() - bar_width) / 2;

    if (sc == SC_ScrollBarGroove) {
        auto bb = QProxyStyle::subControlRect(cc, opt, sc, widget);
        bb.adjust(0, -scroll_bar_edge_offset_, 0, scroll_bar_edge_offset_);
        return bb;
    }

    if (sc == SC_ScrollBarSlider) {
        auto bb = QProxyStyle::subControlRect(cc, opt, sc, widget);
        bb.translate(bar_dx, 0);
        bb.setWidth(qMin(bar_width, opt->rect.width()));
        bb.adjust(0, -scroll_bar_edge_offset_, 0, scroll_bar_edge_offset_);
        return bb;
    }

    return {};
}

ScrollBar::ScrollBar(QWidget *parent)
    : QScrollBar(Qt::Vertical, parent) {
    QStyleOptionSlider opt;
    initStyleOption(&opt);

    const auto bb =
        style()->subControlRect(QStyle::CC_ScrollBar, &opt, QStyle::SC_ScrollBarAddLine, this);

    setStyle(new ScrollBarProxyStyle(style()));

    setEdgeOffset(bb.height());
}

void ScrollBar::setEdgeOffset(int offset) {
    static_cast<ScrollBarProxyStyle *>(style())->scroll_bar_edge_offset_ = qMax(0, offset);
    update();
}

void ScrollBar::paintEvent(QPaintEvent *event) {
    QPainter    p(this);
    const auto &pal = palette();
    p.setRenderHint(QPainter::Antialiasing);

    QStyleOptionSlider opt;
    initStyleOption(&opt);

    p.setPen(pal.color(backgroundRole()));
    p.fillRect(rect(), pal.brush(backgroundRole()));
    p.drawRect(rect());

    const auto bb =
        style()->subControlRect(QStyle::CC_ScrollBar, &opt, QStyle::SC_ScrollBarSlider, this);

    auto color = pal.color(foregroundRole());
    if (!opt.state.testFlag(QStyle::State_MouseOver)) { color.setAlpha(80); }

    p.setPen(color);
    p.setBrush(color);
    p.drawRoundedRect(bb, 4, 4);
}

} // namespace jwrite::ui
