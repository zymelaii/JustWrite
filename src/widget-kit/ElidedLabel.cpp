#include <widget-kit/ElidedLabel.h>
#include <QPaintEvent>
#include <QResizeEvent>

namespace widgetkit {

void ElidedLabel::set_elide_mode(Qt::TextElideMode elide_mode) {
    elide_mode_ = elide_mode;
    cached_text_.clear();
    update();
}

void ElidedLabel::refresh_elided_text() {
    const auto raw_text = text();
    if (cached_text_ == raw_text) { return; }
    cached_text_        = raw_text;
    const auto fm       = fontMetrics();
    cached_elided_text_ = fm.elidedText(cached_text_, elide_mode_, width(), Qt::TextSingleLine);
    if (!cached_text_.isEmpty()) {
        const auto min_text = cached_text_.at(0) + QString("...");
        setMinimumWidth(fm.horizontalAdvance(min_text) + 1);
    }
}

ElidedLabel::ElidedLabel(QWidget *parent, Qt::WindowFlags f)
    : ElidedLabel("", parent, f) {}

ElidedLabel::ElidedLabel(const QString &text, QWidget *parent, Qt::WindowFlags f)
    : QLabel(text, parent, f) {
    set_elide_mode(Qt::ElideRight);
}

void ElidedLabel::paintEvent(QPaintEvent *event) {
    if (elide_mode_ == Qt::ElideNone) { return QLabel::paintEvent(event); }

    refresh_elided_text();
    setText(cached_elided_text_);
    QLabel::paintEvent(event);
    setText(cached_text_);
}

void ElidedLabel::resizeEvent(QResizeEvent *event) {
    QLabel::resizeEvent(event);
    cached_text_.clear();
}

} // namespace widgetkit
