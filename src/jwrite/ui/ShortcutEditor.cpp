#include <jwrite/ui/ShortcutEditor.h>
#include <QPainter>
#include <QMouseEvent>

namespace jwrite::ui {

void ShortcutEditor::set_shortcut(const QKeySequence &shortcut) {
    shortcut_ = shortcut;
    updateGeometry();
    update();
    emit on_shortcut_change(shortcut_);
}

void ShortcutEditor::set_listen_enabled(bool enabled) {
    ui_on_listen_ = enabled;
    updateGeometry();
    update();
}

QString ShortcutEditor::display_text() const {
    if (ui_on_listen_) {
        return "等待输入";
    } else if (shortcut_.isEmpty()) {
        return "未设置";
    } else {
        return shortcut_.toString(QKeySequence::NativeText);
    }
}

ShortcutEditor::ShortcutEditor(QWidget *parent)
    : QWidget(parent)
    , ui_hover_{false}
    , ui_on_listen_{false} {
    init();
}

void ShortcutEditor::init() {
    auto font = this->font();
    font.setPointSize(12);
    setFont(font);

    auto pal = palette();
    pal.setColor(QPalette::Base, "#7a939f");
    pal.setColor(QPalette::Text, "#ffffff");
    pal.setColor(QPalette::Window, "#eeeeee");
    pal.setColor(QPalette::WindowText, "#000000");
    setPalette(pal);

    setContentsMargins(4, 4, 4, 4);

    setFocusPolicy(Qt::ClickFocus);
}

QSize ShortcutEditor::minimumSizeHint() const {
    const auto fm = fontMetrics();
    QRect      bb(QPoint(0, 0), fm.size(0, display_text()));
    bb.adjust(-8, -4, 8, 4);
    return bb.marginsAdded(contentsMargins()).size();
}

QSize ShortcutEditor::sizeHint() const {
    return minimumSizeHint();
}

void ShortcutEditor::enterEvent(QEnterEvent *event) {
    QWidget::enterEvent(event);
    ui_hover_ = true;
    update();
}

void ShortcutEditor::leaveEvent(QEvent *event) {
    QWidget::leaveEvent(event);
    ui_hover_ = false;
    update();
}

void ShortcutEditor::focusOutEvent(QFocusEvent *event) {
    QWidget::focusOutEvent(event);
    if (ui_on_listen_) { set_listen_enabled(false); }
}

void ShortcutEditor::keyPressEvent(QKeyEvent *event) {
    if (!ui_on_listen_) { return; }

    const int  key = event->key();
    const bool allow_single_key =
        key >= Qt::Key_Escape && key <= Qt::Key_PageDown || key >= Qt::Key_F1 && key <= Qt::Key_F12;

    if (!allow_single_key && event->modifiers() == Qt::NoModifier) { return; }
    if (!allow_single_key && !(key >= Qt::Key_Space && key <= Qt::Key_AsciiTilde)) { return; }

    set_listen_enabled(false);
    set_shortcut(QKeySequence(event->keyCombination()));
}

void ShortcutEditor::mousePressEvent(QMouseEvent *event) {
    if (!contentsRect().contains(event->pos())) { return; }
    if (ui_on_listen_) {
        if (event->button() != Qt::LeftButton && event->button() != Qt::RightButton) { return; }
        set_listen_enabled(false);
        if (event->button() == Qt::RightButton) { set_shortcut(QKeySequence()); }
    } else if (event->button() == Qt::LeftButton) {
        set_listen_enabled(true);
    }
}

void ShortcutEditor::paintEvent(QPaintEvent *event) {
    QPainter    p(this);
    const auto &pal = palette();

    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::TextAntialiasing);

    const auto bb   = contentsRect();
    const auto bg   = ui_hover_ ? pal.base().color() : pal.window().color();
    const auto fg   = ui_hover_ ? pal.text().color() : pal.windowText().color();
    const auto text = display_text();

    p.setPen(bg);
    p.setBrush(bg);
    p.drawRoundedRect(bb, 4, 4);

    p.setPen(fg);
    p.drawText(bb, Qt::AlignCenter, text);
}

} // namespace jwrite::ui
