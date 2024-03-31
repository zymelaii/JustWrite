#include "ParaBlockEdit.h"
#include <utility>
#include <QClipboard>
#include <QGuiApplication>
#include <QPaintEvent>
#include <QKeyEvent>
#include <QFocusEvent>
#include <QInputMethodEvent>
#include <QResizeEvent>
#include <QPainter>
#include <QStyle>
#include <QMimeData>

struct ParaBlockEditPrivate {
    QString  text;
    QString  display_text;
    int      cursor_pos;
    int      last_cursor_pos;
    int      sel_begin;
    int      sel_end;
    QMargins text_margins;
    int      total_lines_hint;
    bool     paint_cursor;

    ParaBlockEditPrivate() {
        cursor_pos       = 0;
        last_cursor_pos  = cursor_pos;
        sel_begin        = cursor_pos;
        sel_end          = sel_begin - 1;
        text_margins     = QMargins();
        total_lines_hint = 1;
        paint_cursor     = false;
    }
};

QString leftTrimmed(const QString &s) {
    int start = 0;
    while (start < s.size() && s[start].isSpace()) { ++start; }
    return s.mid(start);
}

ParaBlockEdit::ParaBlockEdit(QWidget *parent)
    : QWidget(parent)
    , d{new ParaBlockEditPrivate} {
    setFocusPolicy(Qt::ClickFocus);
    setAttribute(Qt::WA_InputMethodEnabled);

    auto pal = palette();
    pal.setColor(QPalette::Base, Qt::transparent);
    pal.setColor(QPalette::Text, Qt::white);
    pal.setColor(QPalette::Highlight, QColor(50, 100, 150, 150));
    setPalette(pal);

    setContentsMargins(4, 4, 4, 4);

    connect(this, &ParaBlockEdit::textChanged, this, &ParaBlockEdit::updateDisplayText);
    connect(this, &ParaBlockEdit::cursorPositionChanged, this, &ParaBlockEdit::postUpdateRequest);
    connect(this, &ParaBlockEdit::selTextChanged, this, &ParaBlockEdit::postUpdateRequest);
    connect(this, &ParaBlockEdit::displayTextChanged, this, &ParaBlockEdit::adjustTextRegion);
}

ParaBlockEdit::~ParaBlockEdit() {
    delete d;
}

QSize ParaBlockEdit::sizeHint() const {
    const auto fm          = fontMetrics();
    const auto cm          = contentsMargins();
    const auto tm          = d->text_margins;
    const auto line_height = getTightLineHeight();
    const auto max_width   = contentsRect().width() - tm.left() - tm.right();

    constexpr QChar SAMPLE_CN_CHAR(U'\u6211');
    const auto      leading_width = fm.horizontalAdvance(SAMPLE_CN_CHAR) * 2;

    const QStringView text = d->display_text;

    auto line_width  = max_width - leading_width;
    auto text_start  = 0;
    auto total_lines = 1;

    while (true) {
        int line_chars = getTextLengthInWidth(text.mid(text_start), line_width, nullptr);
        if (text_start + line_chars == text.size()) { break; }
        ++total_lines;
        text_start += line_chars;
        line_width  = max_width;
    }

    return QSize(0, cm.top() + cm.bottom() + (line_height + tm.top() + tm.bottom()) * total_lines);
}

QSize ParaBlockEdit::minimumSizeHint() const {
    const auto fm      = fontMetrics();
    const auto cm      = contentsMargins();
    const auto tm      = d->text_margins;
    const auto margins = cm + tm;
    const auto w       = fm.maxWidth() * 3 + margins.left() + margins.right();
    const auto h       = cm.top() + cm.bottom() + (getTightLineHeight() + tm.top() + tm.bottom()) * d->total_lines_hint;
    return QSize(w, h);
}

void ParaBlockEdit::setCursorPosition(int pos) {
    if (pos == d->cursor_pos) { return; }
    d->cursor_pos      = qMax(0, qMin(pos, d->text.size()));
    d->last_cursor_pos = d->cursor_pos;
    emit cursorPositionChanged(d->cursor_pos);
}

int ParaBlockEdit::cursorPosition() const {
    return d->cursor_pos;
}

QString ParaBlockEdit::text() const {
    return d->text;
}

QString ParaBlockEdit::selectedText() const {
    return hasSelText() ? d->text.mid(d->sel_begin, d->sel_end - d->sel_begin + 1) : "";
}

void ParaBlockEdit::clearText() {
    const auto old_text = std::move(d->text);
    d->text.clear();
    emit textChanged(d->text, old_text);
    setCursorPosition(0);
}

bool ParaBlockEdit::hasSelText() const {
    return d->sel_begin <= d->sel_end;
}

void ParaBlockEdit::clearSelText() {
    if (!hasSelText()) { return; }
    const auto old_text = d->text;
    d->text.remove(d->sel_begin, d->sel_end - d->sel_begin + 1);
    d->display_text = d->text;
    if (d->cursor_pos > d->sel_end) {
        d->cursor_pos -= d->sel_end - d->sel_begin + 1;
    } else if (d->cursor_pos > d->sel_begin) {
        d->cursor_pos = d->sel_begin;
    }
    d->last_cursor_pos = d->cursor_pos;
    d->sel_begin       = d->cursor_pos;
    d->sel_end         = d->sel_begin - 1;
    emit selTextChanged(d->text, d->sel_begin, d->sel_end);
    emit textChanged(d->text, old_text);
}

void ParaBlockEdit::unsetSelection() {
    d->sel_end = d->sel_begin - 1;
    emit selTextChanged(d->text, d->sel_begin, d->sel_end);
}

int ParaBlockEdit::getTextLengthInWidth(QStringView text, int width, int *text_width) const {
    const auto fm        = fontMetrics();
    int        len       = 0;
    const auto max_width = width;
    for (const auto &c : text) {
        const auto char_width = fm.horizontalAdvance(c);
        if (width < char_width) { break; }
        width -= char_width;
        ++len;
    }
    if (text_width) { *text_width = max_width - width; }
    return len;
}

void ParaBlockEdit::insert(const QString &text) {
    clearSelText();
    if (text.isEmpty()) { return; }
    const auto old_text = d->text;
    const auto new_text = d->cursor_pos == 0 ? leftTrimmed(text) : text;
    d->text.insert(d->cursor_pos, new_text);
    d->cursor_pos      += new_text.size();
    d->last_cursor_pos  = d->cursor_pos;
    emit textChanged(d->text, old_text);
}

void ParaBlockEdit::del() {
    Q_ASSERT(d->cursor_pos <= d->text.size());
    if (hasSelText()) {
        clearSelText();
        return;
    }
    if (d->cursor_pos == d->text.size()) { return; }
    const auto old_text = d->text;
    d->text.remove(d->cursor_pos, 1);
    emit textChanged(d->text, old_text);
}

void ParaBlockEdit::backspace() {
    Q_ASSERT(d->cursor_pos >= d->text.size());
    if (hasSelText()) {
        clearSelText();
        return;
    }
    if (d->cursor_pos == 0) { return; }
    const auto old_text = d->text;
    d->text.remove(d->cursor_pos - 1, 1);
    moveLeft();
    emit textChanged(d->text, old_text);
}

void ParaBlockEdit::select(int begin, int end) {
    if (begin > end || d->text.isEmpty()) { return; }
    d->sel_begin = qMax(0, qMin(begin, d->text.size() - 1));
    d->sel_end   = qMax(0, qMin(end, d->text.size() - 1));
    emit selTextChanged(d->text, d->sel_begin, d->sel_end);
}

void ParaBlockEdit::selectAll() {
    if (d->text.isEmpty()) { return; }
    d->sel_begin = 0;
    d->sel_end   = d->text.size() - 1;
    emit selTextChanged(d->text, d->sel_begin, d->sel_end);
}

void ParaBlockEdit::copy() {
    const auto copied_text = hasSelText() ? selectedText() : text();
    QGuiApplication::clipboard()->setText(copied_text);
}

void ParaBlockEdit::paste() {
    auto clipboard = QGuiApplication::clipboard();
    auto mime      = clipboard->mimeData();
    if (!mime->hasText()) { return; }
    insert(mime->text());
}

void ParaBlockEdit::cut() {
    copy();
    if (hasSelText()) {
        clearSelText();
    } else {
        unsetSelection();
        clearText();
    }
}

void ParaBlockEdit::moveLeft() {
    d->cursor_pos      = qMax(0, d->cursor_pos - 1);
    d->last_cursor_pos = d->cursor_pos;
    unsetSelection();
    emit cursorPositionChanged(d->cursor_pos);
}

void ParaBlockEdit::moveRight() {
    d->cursor_pos      = qMin(d->cursor_pos + 1, d->text.size());
    d->last_cursor_pos = d->cursor_pos;
    unsetSelection();
    emit cursorPositionChanged(d->cursor_pos);
}

void ParaBlockEdit::moveDown() {}

void ParaBlockEdit::moveUp() {}

void ParaBlockEdit::gotoStartOfBlock() {
    d->cursor_pos      = 0;
    d->last_cursor_pos = d->cursor_pos;
    unsetSelection();
    emit cursorPositionChanged(d->cursor_pos);
}

void ParaBlockEdit::gotoEndOfBlock() {
    d->cursor_pos      = d->text.size();
    d->last_cursor_pos = d->cursor_pos;
    unsetSelection();
    emit cursorPositionChanged(d->cursor_pos);
}

void ParaBlockEdit::resizeEvent(QResizeEvent *e) {
    QWidget::resizeEvent(e);
    adjustTextRegion();
}

void ParaBlockEdit::paintEvent(QPaintEvent *e) {
    QPainter p(this);

    auto pal = palette();
    p.setPen(pal.text().color());

#ifndef NDEBUG
    p.fillRect(rect(), QBrush(QColor(255, 150, 55)));
    p.fillRect(contentsRect(), QBrush(QColor(80, 80, 100)));
#endif

    constexpr QChar SAMPLE_CN_CHAR(U'\u6211');

    const auto fm = fontMetrics();
    const auto cm = contentsMargins();
    const auto tm = d->text_margins;

    const auto leading_width = fm.boundingRect(SAMPLE_CN_CHAR).width() * 2;
    const auto origin_x      = contentsRect().x() + tm.left();
    const auto origin_y      = contentsRect().y() + tm.top();
    const auto max_width     = contentsRect().width() - tm.left() - tm.right();
    const auto height        = getTightLineHeight();

    int i = 0, j = 0;
    int x = origin_x + leading_width;
    int y = origin_y;

    auto text_start = 0;
    auto width      = max_width - leading_width;

    const auto        flags = Qt::AlignBaseline | Qt::TextDontClip;
    const QStringView text  = d->display_text;

    int    char_pos = 0;
    QPoint cursor_pos(x, y);

    //! draw active para highlight
    if (hasFocus()) { p.fillRect(rect(), QBrush(QColor(255, 255, 255, 10))); }

    //! draw text
    while (text_start < text.size()) {
        int          line_width = -1;
        const int    line_chars = getTextLengthInWidth(text.mid(text_start), width, &line_width);
        const double mean_width = text_start + line_chars == text.size() ? 0 : width - line_width;
        const double width_incr = line_chars < 2 ? 0 : mean_width / (line_chars - 1);

        QRectF bb_exact(x, y, max_width - (x - origin_x), height);
        QRectF bb_char;

        for (auto &c : text.mid(text_start, line_chars)) {
            p.drawText(bb_exact, flags, c, &bb_char);
            if (d->cursor_pos == ++char_pos) { cursor_pos = QPoint(bb_char.right(), y); }
            bb_exact.setLeft(bb_exact.left() + bb_char.width() + width_incr);
        }

        width  = max_width;
        x      = origin_x;
        y     += tm.top() + tm.bottom() + height;

        text_start += line_chars;
    }

    //! draw cursor
    const auto cursor_width = style()->pixelMetric(QStyle::PM_TextCursorWidth);
    if (d->paint_cursor) { p.fillRect(cursor_pos.x(), cursor_pos.y(), cursor_width, height - fm.descent(), Qt::white); }
}

void ParaBlockEdit::keyPressEvent(QKeyEvent *e) {
    if (auto text = e->text(); !text.isEmpty() && text.at(0).isPrint()) { insert(text.at(0)); }

    if (false) {
    } else if (e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter) {
        emit finished();
    } else if (e->key() == Qt::Key_Backspace && !(e->modifiers() & ~(Qt::ControlModifier | Qt::AltModifier))) {
        backspace();
    } else if (e->matches(QKeySequence::Delete)) {
        del();
    } else if (e->matches(QKeySequence::MoveToPreviousChar)) {
        moveLeft();
    } else if (e->matches(QKeySequence::MoveToNextChar)) {
        moveRight();
    } else if (e->matches(QKeySequence::MoveToStartOfLine)) {
        gotoStartOfBlock();
    } else if (e->matches(QKeySequence::MoveToEndOfLine)) {
        gotoEndOfBlock();
    } else if (e->matches(QKeySequence::SelectAll)) {
        selectAll();
    } else if (e->matches(QKeySequence::Copy)) {
        copy();
    } else if (e->matches(QKeySequence::Paste)) {
        paste();
    } else if (e->matches(QKeySequence::Cut)) {
        cut();
    } else if (e->key() == Qt::Key_Escape && hasSelText()) {
        unsetSelection();
    }

    e->accept();
}

void ParaBlockEdit::focusInEvent(QFocusEvent *e) {
    QWidget::focusInEvent(e);
    d->paint_cursor = true;
    emit gotFocus();
}

void ParaBlockEdit::focusOutEvent(QFocusEvent *e) {
    QWidget::focusOutEvent(e);
    d->paint_cursor = false;
    if (d->text.endsWith(QChar::Space)) {
        const auto old_text = d->text;
        while (d->text.endsWith(QChar::Space)) { d->text.chop(1); }
        if (d->cursor_pos > d->text.size()) {
            d->cursor_pos      = d->text.size();
            d->last_cursor_pos = d->cursor_pos;
            emit cursorPositionChanged(d->cursor_pos);
        }
        emit textChanged(d->text, old_text);
    }
    emit lostFocus();
}

void ParaBlockEdit::inputMethodEvent(QInputMethodEvent *e) {
    clearSelText();
    if (!e->preeditString().isEmpty()) {
        d->display_text = d->text;
        d->display_text.insert(d->last_cursor_pos, e->preeditString());
        d->cursor_pos = d->last_cursor_pos + e->preeditString().size();
        emit displayTextChanged(d->display_text);
    } else {
        const auto old_text = d->text;
        d->text.insert(d->last_cursor_pos, e->commitString());
        d->cursor_pos      = d->last_cursor_pos + e->commitString().size();
        d->last_cursor_pos = d->cursor_pos;
        emit textChanged(d->text, old_text);
    }
}

QVariant ParaBlockEdit::inputMethodQuery(Qt::InputMethodQuery query) const {
    return QWidget::inputMethodQuery(query);
}

int ParaBlockEdit::getTightLineHeight() const {
    const auto fm = fontMetrics();
    return fm.height() + fm.descent();
}

void ParaBlockEdit::updateDisplayText(const QString &text) {
    d->display_text = text;
    emit displayTextChanged(d->display_text);
}

void ParaBlockEdit::postUpdateRequest() {
    update();
}

void ParaBlockEdit::adjustTextRegion() {
    updateGeometry();
    update();
}
