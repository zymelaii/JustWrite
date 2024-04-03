#include "LimitedViewEditor.h"
#include <QResizeEvent>
#include <QPaintEvent>
#include <QFocusEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QInputMethodEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QPainter>
#include <QGuiApplication>
#include <QClipboard>
#include <QTimer>

struct LimitedViewEditorPrivate {
    int    min_text_line_chars;
    double text_line_spacing_ratio;
    int    para_block_spacing;

    int cursor_pos;

    bool preedit;
    int  preedit_start_pos;

    int active_para_index;

    bool align_center;

    double scroll;

    bool   blink_cursor_should_paint;
    QTimer blink_timer;

    QString                  text;
    QVector<TextBlockLayout> paras;

    LimitedViewEditorPrivate() {
        min_text_line_chars     = 12;
        text_line_spacing_ratio = 1.0;
        para_block_spacing      = 6;

        cursor_pos = 0;

        preedit           = false;
        preedit_start_pos = -1;

        active_para_index = -1;

        align_center = false;

        scroll = 0.0;

        blink_cursor_should_paint = true;
        blink_timer.setInterval(500);
        blink_timer.setSingleShot(false);
    }
};

static QString leftTrimmed(const QString &s) {
    int start = 0;
    while (start < s.size() && s[start].isSpace()) { ++start; }
    return s.mid(start);
}

LimitedViewEditor::LimitedViewEditor(QWidget *parent)
    : QWidget(parent)
    , d{new LimitedViewEditorPrivate} {
    setFocusPolicy(Qt::ClickFocus);
    setAttribute(Qt::WA_InputMethodEnabled);
    setAcceptDrops(true);

    setAlignCenter(false);

    connect(this, &LimitedViewEditor::textAreaChanged, this, [this](QRect area) {
        if (d->paras.empty()) { return; }
        if (auto max_width = area.width(); max_width != d->paras[0].max_width) { updateTextBlockMaxWidth(max_width); }
        update();
    });
    connect(&d->blink_timer, &QTimer::timeout, this, [this] {
        d->blink_cursor_should_paint = !d->blink_cursor_should_paint;
        update();
    });
}

LimitedViewEditor::~LimitedViewEditor() {
    delete d;
}

QSize LimitedViewEditor::minimumSizeHint() const {
    const auto fm          = fontMetrics();
    const auto char_width  = fm.horizontalAdvance(QChar(U'\u3000'));
    const auto margins     = contentsMargins();
    const auto line_height = fm.height() + fm.descent();
    const auto hori_margin = margins.left() + margins.right();
    const auto vert_margin = margins.top() + margins.bottom();
    const auto min_width   = d->min_text_line_chars * char_width + hori_margin;
    const auto min_height  = d->text_line_spacing_ratio * line_height * 3 + d->para_block_spacing * 2 + vert_margin;
    return QSize(min_width, min_height);
}

QSize LimitedViewEditor::sizeHint() const {
    return minimumSizeHint();
}

int LimitedViewEditor::paraBlockSpacing() const {
    return d->para_block_spacing;
}

void LimitedViewEditor::setParaBlockSpacing(int spacing) {
    d->para_block_spacing = qMax(0, spacing);
    update();
}

int LimitedViewEditor::minTextLineChars() const {
    return d->min_text_line_chars;
}

void LimitedViewEditor::setMinTextLineChars(int min_chars) {
    d->min_text_line_chars = qMax(4, min_chars);
    updateGeometry();
    update();
}

double LimitedViewEditor::textLineSpacingRatio() const {
    return d->text_line_spacing_ratio;
}

void LimitedViewEditor::setTextLineSpacingRatio(double ratio) {
    d->text_line_spacing_ratio = qBound(0.75, ratio, 2.5);
    update();
}

bool LimitedViewEditor::alignCenter() const {
    return d->align_center;
}

void LimitedViewEditor::setAlignCenter(bool value) {
    d->align_center = value;
    if (d->align_center) {
        const auto min_margin     = 32;
        const auto max_text_width = 1000;
        const auto mean_width     = qMax(0, width() - min_margin * 2);
        const auto text_width     = qMin<int>(mean_width * 0.8, max_text_width);
        const auto margin         = (width() - text_width) / 2;
        setContentsMargins(margin, 4, margin, 4);
    } else {
        setContentsMargins(4, 4, 4, 4);
    }
    emit textAreaChanged(textArea());
}

QRect LimitedViewEditor::textArea() const {
    return rect() - contentsMargins();
}

TextBlockLayout &LimitedViewEditor::currentTextBlock() {
    if (d->active_para_index == -1) {
        TextBlockLayout para;
        para.reset(d->text, d->cursor_pos);
        para.setMaxWidth(textArea().width());
        d->paras.push_back(para);
        switchTextBlock(0);
    }
    return d->paras[d->active_para_index];
}

void LimitedViewEditor::switchTextBlock(int index) {
    if (index < 0 || index >= d->paras.size() || index == d->active_para_index) { return; }
    if (d->active_para_index != -1) {
        auto &para = currentTextBlock();
        if (para.isEmpty()) {
            if (index > d->active_para_index) { --index; }
            d->paras.remove(d->active_para_index);
        }
    }
    d->active_para_index = index;
    update();
}

void LimitedViewEditor::insertMultiLineText(const QString &text) {
    auto lines = text.split("\n");
    insert(lines.front());
    for (int i = 1; i < lines.size(); ++i) {
        const auto text = lines[i].trimmed();
        if (text.isEmpty()) { continue; }
        TextBlockLayout para;
        para.reset(d->text, d->cursor_pos);
        para.setMaxWidth(textArea().width());
        d->paras.push_back(para);
        switchTextBlock(d->paras.size() - 1);
        insert(text);
    }
}

void LimitedViewEditor::scroll(double delta) {
    const auto fm          = fontMetrics();
    const auto line_height = fm.height() + fm.descent();

    double max_scroll = d->text_line_spacing_ratio * line_height + d->para_block_spacing;
    double min_scroll = 0;

    for (auto &para : d->paras) {
        min_scroll -= para.lines.size() * d->text_line_spacing_ratio * line_height + d->para_block_spacing;
    }
    min_scroll += d->text_line_spacing_ratio * line_height + d->para_block_spacing;

    d->scroll = qBound<double>(min_scroll, d->scroll + delta, max_scroll);

    update();
}

void LimitedViewEditor::insert(const QString &text) {
    //! ATTENTION: must get block first to ensure the origin is right
    auto &para = currentTextBlock().sync(d->text);
    //! TODO: ensure no sel
    const auto inserted = para.cursor_pos == 0 ? leftTrimmed(text) : text;
    d->text.insert(d->cursor_pos, inserted);
    const auto size  = inserted.length();
    d->cursor_pos   += size;
    para.quickInsert(size);
    for (int i = d->active_para_index + 1; i < d->paras.size(); ++i) { d->paras[i].shiftOrigin(size); }
    update();
}

void LimitedViewEditor::deleteForward() {
    auto &para = currentTextBlock();
    if (para.deleteForward()) {
        d->text.remove(d->cursor_pos, 1);
        for (int i = d->active_para_index + 1; i < d->paras.size(); ++i) { d->paras[i].shiftOrigin(-1); }
    } else if (d->active_para_index + 1 != d->paras.size()) {
        para.mergeNextBlock(d->paras[d->active_para_index + 1]);
        d->paras.remove(d->active_para_index + 1);
    }
    update();
}

void LimitedViewEditor::deleteBackward() {
    auto &para = currentTextBlock();
    if (para.deleteBackward()) {
        d->text.remove(d->cursor_pos, 1);
        --d->cursor_pos;
        for (int i = d->active_para_index + 1; i < d->paras.size(); ++i) { d->paras[i].shiftOrigin(-1); }
    } else if (d->active_para_index > 0) {
        para.joinPrevBlock(d->paras[d->active_para_index - 1]);
        switchTextBlock(d->active_para_index - 1);
        d->paras.remove(d->active_para_index + 1);
    }
    update();
}

void LimitedViewEditor::paste() {
    auto clipboard = QGuiApplication::clipboard();
    auto mime      = clipboard->mimeData();
    if (!mime->hasText()) { return; }
    //! TODO: optimize large text paste
    insertMultiLineText(mime->text());
}

void LimitedViewEditor::moveToPreviousChar() {
    auto &para = currentTextBlock();
    --d->cursor_pos;
    if (para.cursor_col > 0) {
        --para.cursor_col;
        --para.cursor_pos;
    } else if (para.cursor_row > 0) {
        --para.cursor_row;
        para.cursor_col = para.line(para.cursor_row).length() - 1;
        --para.cursor_pos;
    } else if (d->active_para_index > 0) {
        switchTextBlock(d->active_para_index - 1);
        auto &para      = currentTextBlock();
        para.cursor_row = para.lines.size() - 1;
        para.cursor_col = para.line(para.cursor_row).length();
        para.cursor_pos = para.lines.back().text_endp - para.text_pos;
        ++d->cursor_pos;
    } else {
        ++d->cursor_pos;
    }
    d->blink_cursor_should_paint = true;
    update();
}

void LimitedViewEditor::moveToNextChar() {
    auto &para = currentTextBlock();
    ++d->cursor_pos;
    if (para.cursor_col < para.line(para.cursor_row).length()) {
        ++para.cursor_col;
        ++para.cursor_pos;
    } else if (para.cursor_row < para.lines.size() && !para.line(para.cursor_row + 1).isEmpty()) {
        ++para.cursor_row;
        para.cursor_col = 1;
        ++para.cursor_pos;
    } else if (d->active_para_index + 1 < d->paras.size()) {
        switchTextBlock(d->active_para_index + 1);
        auto &para      = currentTextBlock();
        para.cursor_row = 0;
        para.cursor_col = 0;
        para.cursor_pos = 0;
        --d->cursor_pos;
    } else {
        --d->cursor_pos;
    }
    d->blink_cursor_should_paint = true;
    update();
}

void LimitedViewEditor::moveToEndOfLine() {
    auto &para = currentTextBlock();
    if (auto size = para.line(para.cursor_row).length(); para.cursor_col < size) {
        int offset       = size - para.cursor_col;
        para.cursor_pos += offset;
        para.cursor_col  = size;
        d->cursor_pos   += offset;
    } else {
        int offset       = para.lines.back().text_endp - para.text_pos - para.cursor_pos;
        para.cursor_pos += offset;
        para.cursor_row  = para.lines.size() - 1;
        para.cursor_col  = para.line(para.cursor_row).length();
        d->cursor_pos   += offset;
    }
    d->blink_cursor_should_paint = true;
    update();
}

void LimitedViewEditor::moveToStartOfLine() {
    auto &para = currentTextBlock();
    if (para.cursor_col > 0) {
        d->cursor_pos   -= para.cursor_col;
        para.cursor_pos -= para.cursor_col;
        para.cursor_col  = 0;
    } else {
        d->cursor_pos   -= para.cursor_pos;
        para.cursor_col  = 0;
        para.cursor_row  = 0;
        para.cursor_pos  = 0;
    }
    d->blink_cursor_should_paint = true;
    update();
}

void LimitedViewEditor::moveToPreviousLine() {
    auto &para   = currentTextBlock();
    int   offset = -1;
    if (para.cursor_row > 0) {
        const auto col = para.cursor_col;
        --para.cursor_row;
        const auto len   = para.lengthOfLine(para.cursor_row);
        para.cursor_col  = qMin(col, len);
        offset           = len - para.cursor_col + col;
        para.cursor_pos -= offset;
    } else if (d->active_para_index != 0) {
        switchTextBlock(d->active_para_index - 1);
        const auto col  = para.cursor_col;
        auto      &para = currentTextBlock();
        para.cursor_row = para.lines.size() - 1;
        const auto len  = para.lengthOfLine(para.cursor_row);
        para.cursor_col = qMin(col, len);
        offset          = len - para.cursor_col + col;
        para.cursor_pos = para.lines.back().text_endp - para.text_pos - (len - para.cursor_col);
    }
    if (offset != -1) { d->cursor_pos -= offset; }
    d->blink_cursor_should_paint = true;
    update();
}

void LimitedViewEditor::moveToNextLine() {
    auto &para   = currentTextBlock();
    int   offset = -1;
    if (para.cursor_row + 1 < para.lines.size()) {
        const auto col  = para.cursor_col;
        const auto mean = para.lengthOfLine(para.cursor_row) - col;
        ++para.cursor_row;
        const auto len   = para.lengthOfLine(para.cursor_row);
        para.cursor_col  = qMin(col, len);
        offset           = mean + para.cursor_col;
        para.cursor_pos += offset;
    } else if (d->active_para_index < d->paras.size()) {
        switchTextBlock(d->active_para_index + 1);
        const auto col  = para.cursor_col;
        const auto mean = para.lengthOfLine(para.cursor_row) - col;
        auto      &para = currentTextBlock();
        para.cursor_row = 0;
        const auto len  = para.lengthOfLine(para.cursor_row);
        para.cursor_col = qMin(col, len);
        para.cursor_pos = para.cursor_col;
        offset          = mean + para.cursor_col;
    }
    if (offset != -1) { d->cursor_pos += offset; }
    d->blink_cursor_should_paint = true;
    update();
}

void LimitedViewEditor::splitIntoNewLine() {
    //! FIXME: dead recursion
    Q_ASSERT(!d->paras.isEmpty());
    auto para = currentTextBlock().split();
    d->paras.insert(d->active_para_index + 1, para);
    switchTextBlock(d->active_para_index + 1);
    d->blink_cursor_should_paint = true;
    update();
}

void LimitedViewEditor::updateTextBlockMaxWidth(int max_width) {
    for (auto &para : d->paras) {
        para.setMaxWidth(max_width);
        para.render(fontMetrics());
    }
    //! TODO: maybe need more or less text blocks
}

void LimitedViewEditor::resizeEvent(QResizeEvent *e) {
    QWidget::resizeEvent(e);
    if (d->align_center) { setAlignCenter(true); }
    emit textAreaChanged(textArea());
}

void LimitedViewEditor::paintEvent(QPaintEvent *e) {
    //! FIXME: better not to render in paint event
    for (auto &para : d->paras) { para.sync(d->text).render(fontMetrics()); }

    QPainter p(this);
    auto     pal = palette();

    //! draw margins
    p.fillRect(rect(), QColor(30, 30, 30));

    //! draw test area
    p.setPen(pal.text().color());

    const auto   fm           = fontMetrics();
    const double para_spacing = d->para_block_spacing;
    const double line_height  = fm.height() + fm.descent();
    const double line_spacing = line_height * d->text_line_spacing_ratio;
    const double y_offset     = 0.0;

    const auto flags     = Qt::AlignBaseline | Qt::TextDontClip;
    const auto text_area = textArea();
    double     y_pos     = text_area.top() + d->scroll;

    for (auto &para : d->paras) {
        for (int i = 0; i < para.lines.size(); ++i) {
            const auto &line = para.lines[i];
            const auto  text = para.line(i);
            const auto  incr = text.length() < 2 ? 0.0 : line.mean_width * 1.0 / (text.length() - 1);
            QRectF      bb(text_area.left() + line.offset, y_pos, para.max_width - line.offset, line_height);
            for (auto &c : text) {
                p.drawText(bb, flags, c);
                bb.setLeft(bb.left() + incr + fm.horizontalAdvance(c));
            }
            y_pos += line_spacing;
        }
        y_pos += para_spacing;
    }

    //! draw cursor
    if (hasFocus() && !d->paras.isEmpty() && d->blink_cursor_should_paint) {
        const auto &para         = currentTextBlock();
        const auto &line         = para.lines[para.cursor_row];
        const auto  text         = para.line(para.cursor_row);
        const auto  incr         = text.length() < 2 ? 0.0 : line.mean_width * 1.0 / (text.length() - 1);
        const auto  text_width   = fm.horizontalAdvance(text.mid(0, para.cursor_col).toString());
        double      cursor_x_pos = text_area.left() + line.offset + text_width + para.cursor_col * incr;
        double      cursor_y_pos = text_area.top() + d->scroll;
        for (int i = 0; i < d->active_para_index; ++i) {
            cursor_y_pos += line_height * d->paras[i].lines.size() + para_spacing;
        }
        cursor_y_pos += para.cursor_row * line_height;
        p.drawLine(cursor_x_pos, cursor_y_pos, cursor_x_pos, cursor_y_pos + fm.height());
    }
}

void LimitedViewEditor::focusInEvent(QFocusEvent *e) {
    QWidget::focusInEvent(e);
    d->blink_cursor_should_paint = true;
    d->blink_timer.start();
    //! force to create one para block
    const auto _ = currentTextBlock();
}

void LimitedViewEditor::focusOutEvent(QFocusEvent *e) {
    QWidget::focusOutEvent(e);
    d->blink_timer.stop();
}

void LimitedViewEditor::keyPressEvent(QKeyEvent *e) {
    if (auto text = e->text(); !text.isEmpty() && text.at(0).isPrint()) {
        insert(text.at(0));
    } else if (auto key = e->key() | e->modifiers(); key == Qt::Key_Return || key == Qt::Key_Enter) {
        splitIntoNewLine();
    } else if (e->matches(QKeySequence::Paste)) {
        paste();
    } else if (e->matches(QKeySequence::MoveToNextPage)) {
        scroll(-textArea().height() * 0.5);
    } else if (e->matches(QKeySequence::MoveToPreviousPage)) {
        scroll(textArea().height() * 0.5);
    } else if (e->matches(QKeySequence::MoveToNextChar)) {
        moveToNextChar();
    } else if (e->matches(QKeySequence::MoveToPreviousChar)) {
        moveToPreviousChar();
    } else if (e->matches(QKeySequence::MoveToEndOfLine)) {
        moveToEndOfLine();
    } else if (e->matches(QKeySequence::MoveToStartOfLine)) {
        moveToStartOfLine();
    } else if (e->matches(QKeySequence::Delete)) {
        deleteForward();
    } else if ((e->key() | e->modifiers()) == Qt::Key_Backspace) {
        deleteBackward();
    } else if (e->matches(QKeySequence::MoveToNextLine)) {
        moveToNextLine();
    } else if (e->matches(QKeySequence::MoveToPreviousLine)) {
        moveToPreviousLine();
    }
    e->accept();
}

void LimitedViewEditor::mousePressEvent(QMouseEvent *e) {
    QWidget::mousePressEvent(e);
}

void LimitedViewEditor::mouseReleaseEvent(QMouseEvent *e) {
    QWidget::mouseReleaseEvent(e);
}

void LimitedViewEditor::mouseDoubleClickEvent(QMouseEvent *e) {
    QWidget::mouseDoubleClickEvent(e);
}

void LimitedViewEditor::mouseMoveEvent(QMouseEvent *e) {
    QWidget::mouseMoveEvent(e);
}

void LimitedViewEditor::wheelEvent(QWheelEvent *e) {
    if (d->paras.empty()) { return; }

    const auto fm          = fontMetrics();
    const auto line_height = fm.height() + fm.descent();

    const double ratio = 1.0 / 180 * 3;
    const auto   delta = e->angleDelta().y() * line_height * ratio;

    scroll(delta);
}

void LimitedViewEditor::inputMethodEvent(QInputMethodEvent *e) {
    if (!e->preeditString().isEmpty()) {
        //! TODO: display preedit string
    } else {
        insert(e->commitString());
    }
}

void LimitedViewEditor::dragEnterEvent(QDragEnterEvent *e) {
    if (e->mimeData()->hasUrls()) {
        e->acceptProposedAction();
    } else {
        e->ignore();
    }
}

void LimitedViewEditor::dropEvent(QDropEvent *e) {
    QWidget::dropEvent(e);
    const auto urls = e->mimeData()->urls();
    //! TODO: filter plain text files and handle open action
}
