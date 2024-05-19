#include <jwrite/ui/TitleBar.h>
#include <QPainter>

namespace jwrite::Ui {

TitleBar::TitleBar(QWidget *parent)
    : QWidget(parent) {
    setupUi();
}

TitleBar::~TitleBar() {}

QString TitleBar::title() const {
    return title_;
}

void TitleBar::setTitle(const QString &title) {
    title_ = title;
    update();
}

QSize TitleBar::minimumSizeHint() const {
    const auto margins      = contentsMargins();
    const auto fm           = fontMetrics();
    const int  hori_spacing = 4;
    const int  height       = fm.height() + fm.descent() + margins.top() + margins.bottom();
    return QSize(0, height + hori_spacing * 2);
}

QSize TitleBar::sizeHint() const {
    return minimumSizeHint();
}

void TitleBar::setupUi() {
    auto font = this->font();
    font.setPointSize(10);
    setFont(font);

    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
}

void TitleBar::paintEvent(QPaintEvent *event) {
    QPainter   p(this);
    const auto pal = palette();

    p.fillRect(rect(), pal.window());

    p.setPen(pal.windowText().color());
    p.drawText(contentsRect(), Qt::AlignCenter, title_);
}

void TitleBar::resizeEvent(QResizeEvent *event) {
    update();
}

} // namespace jwrite::Ui
