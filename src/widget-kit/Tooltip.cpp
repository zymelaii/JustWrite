#include <widget-kit/ToolTip.h>
#include <QApplication>
#include <QMouseEvent>
#include <QPainter>
#include <memory>

namespace widgetkit {

ToolTipLabel::ToolTipLabel()
    : QWidget(nullptr, Qt::ToolTip) {
    setMouseTracking(true);

    qApp->installEventFilter(this);

    setFixedSize(128, 32);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
}

void ToolTipLabel::show_tip(const QPoint &pos, const QString &text) {
    hide_tip();
    if (text.isEmpty()) { return; }
    pos_  = pos;
    text_ = text;
    setGeometry(QRect(pos, size()));
    setVisible(true);
    update();
}

void ToolTipLabel::hide_tip() {
    setVisible(false);
}

ToolTipLabel &ToolTipLabel::get_instance() {
    static std::unique_ptr<ToolTipLabel> instance{};
    if (!instance) { instance.reset(new ToolTipLabel); }
    return *instance;
}

bool ToolTipLabel::eventFilter(QObject *watched, QEvent *event) {
    if (event->type() == QEvent::ToolTip) { hide_tip(); }
    return false;
}

void ToolTipLabel::mouseMoveEvent(QMouseEvent *event) {
    if (!rect_.isNull()) {
        QPoint pos = event->globalPosition().toPoint();
        if (widget_) { pos = widget_->mapFromGlobal(pos); }
        if (!rect_.contains(pos)) { hide_tip(); }
    }
}

void ToolTipLabel::paintEvent(QPaintEvent *event) {
    if (text_.isEmpty()) { return; }

    qDebug() << contentsRect();

    QPainter p(this);
    p.setRenderHint(QPainter::TextAntialiasing);

    p.drawText(QPoint(0, 0), text_);
}

void ToolTip::show_text(const QPoint &pos, const QString &text) {
    ToolTipLabel::get_instance().show_tip(pos, text);
}

} // namespace widgetkit
