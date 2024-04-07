#include "LimitedViewEditor.h"
#include "TextViewEngine.h"
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
#include <QMap>

struct LimitedViewEditorPrivate {
    int                     min_text_line_chars;
    bool                    align_center;
    double                  scroll;
    bool                    edit_op_happens;
    bool                    blink_cursor_should_paint;
    QTimer                  blink_timer;
    int                     cursor_pos;
    QString                 text;
    QString                 preedit_text;
    jwrite::TextViewEngine *engine;

    LimitedViewEditorPrivate() {
        engine                    = nullptr;
        min_text_line_chars       = 12;
        cursor_pos                = 0;
        align_center              = false;
        scroll                    = 0.0;
        edit_op_happens           = false;
        blink_cursor_should_paint = true;
        blink_timer.setInterval(500);
        blink_timer.setSingleShot(false);
    }

    ~LimitedViewEditorPrivate() {
        Q_ASSERT(engine);
        delete engine;
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
    setMouseTracking(true);

    setAlignCenter(true);

    d->engine = new jwrite::TextViewEngine(fontMetrics(), textArea().width());

    scrollToStart();

    connect(this, &LimitedViewEditor::textAreaChanged, this, [this](QRect area) {
        d->engine->resetMaxWidth(area.width());
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
    const auto margins      = contentsMargins();
    const auto line_spacing = d->engine->line_height * d->engine->line_spacing_ratio;
    const auto hori_margin  = margins.left() + margins.right();
    const auto vert_margin  = margins.top() + margins.bottom();
    const auto min_width    = d->min_text_line_chars * d->engine->standard_char_width + hori_margin;
    const auto min_height   = line_spacing * 3 + d->engine->block_spacing * 2 + vert_margin;
    return QSize(min_width, min_height);
}

QSize LimitedViewEditor::sizeHint() const {
    return minimumSizeHint();
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

void LimitedViewEditor::scroll(double delta) {
    const auto e            = d->engine;
    const auto line_spacing = e->line_spacing_ratio * e->line_height;
    double     max_scroll   = line_spacing + e->block_spacing;
    double     min_scroll   = 0;
    for (auto block : e->active_blocks) {
        min_scroll -= block->lines.size() * line_spacing + e->block_spacing;
    }
    min_scroll += line_spacing + e->block_spacing;
    d->scroll   = qBound<double>(min_scroll, d->scroll + delta, max_scroll);
    update();
}

void LimitedViewEditor::scrollToStart() {
    const auto   e            = d->engine;
    const auto   line_spacing = e->line_spacing_ratio * e->line_height;
    const double max_scroll   = line_spacing + e->block_spacing;
    d->scroll                 = max_scroll;
    postUpdateRequest();
}

void LimitedViewEditor::scrollToEnd() {
    const auto e            = d->engine;
    const auto line_spacing = e->line_spacing_ratio * e->line_height;
    double     min_scroll   = 0;
    for (auto block : e->active_blocks) {
        min_scroll -= block->lines.size() * line_spacing + e->block_spacing;
    }
    min_scroll += line_spacing + e->block_spacing;
    d->scroll   = min_scroll;
    postUpdateRequest();
}

void LimitedViewEditor::insertDirtyText(const QString &text) {
    auto text_list = text.split('\n');
    for (int i = 0; i < text_list.size(); ++i) {
        auto text = text_list[i].trimmed();
        if (text.isEmpty()) { continue; }
        insert(text);
        splitIntoNewLine();
    }
}

bool LimitedViewEditor::insertedPairFilter(const QString &text) {
    static QMap<QString, QString> QUOTE_PAIRS{
        {"“", "”"},
        {"‘", "’"},
        {"（", "）"},
        {"【", "】"},
        {"《", "》"},
        {"〔", "〕"},
        {"〈", "〉"},
        {"「", "」"},
        {"『", "』"},
        {"〖", "〗"},
        {"［", "］"},
        {"｛", "｝"},
    };

    if (QUOTE_PAIRS.count(text)) {
        auto matched = QUOTE_PAIRS[text];
        insert(text + matched);
        move(-1);
        return true;
    }

    if (auto index = QUOTE_PAIRS.values().indexOf(text); index != -1) {
        int pos = d->engine->currentBlock()->text_pos + d->engine->cursor.pos;
        if (pos == d->engine->text_ref->length()) { return false; }
        if (d->engine->text_ref->at(pos) == text) { return true; }
    }

    return false;
}

void LimitedViewEditor::move(int offset) {
    bool      cursor_moved  = false;
    const int text_offset   = d->engine->commitMovement(offset, &cursor_moved);
    d->cursor_pos          += text_offset;
    if (cursor_moved) { postUpdateRequest(); }
}

void LimitedViewEditor::insert(const QString &text) {
    Q_ASSERT(d->engine->isCursorAvailable());

    if (insertedPairFilter(text)) { return; }

    d->text.insert(d->cursor_pos, text);
    const int len  = text.length();
    d->cursor_pos += len;
    d->engine->commitInsertion(len);

    postUpdateRequest();
}

void LimitedViewEditor::del(int times) {
    //! NOTE: times means delete |times| chars, and the sign indicates the del direction
    //! TODO: delete selected text
    int       deleted  = 0;
    const int offset   = d->engine->commitDeletion(times, deleted);
    d->cursor_pos     += offset;
    d->text.remove(d->cursor_pos, deleted);
    postUpdateRequest();
}

void LimitedViewEditor::copy() {
    if (!d->engine->isCursorAvailable()) { return; }
    auto clipboard  = QGuiApplication::clipboard();
    auto block_text = d->engine->currentBlock()->text().toString();
    clipboard->setText(block_text);
}

void LimitedViewEditor::cut() {
    copy();
    move(-d->engine->cursor.pos);
    del(d->engine->currentBlock()->textLength());
}

void LimitedViewEditor::paste() {
    auto clipboard = QGuiApplication::clipboard();
    auto mime      = clipboard->mimeData();
    if (!mime->hasText()) { return; }
    insertDirtyText(clipboard->text());
}

void LimitedViewEditor::splitIntoNewLine() {
    d->engine->breakBlockAtCursorPos();
    postUpdateRequest();
}

void LimitedViewEditor::postUpdateRequest() {
    d->blink_cursor_should_paint = true;
    d->edit_op_happens           = true;
    d->blink_timer.stop();
    update();
    d->blink_timer.start();
}

void LimitedViewEditor::resizeEvent(QResizeEvent *e) {
    QWidget::resizeEvent(e);
    if (d->align_center) { setAlignCenter(true); }
    emit textAreaChanged(textArea());
}

void LimitedViewEditor::paintEvent(QPaintEvent *e) {
    auto engine = d->engine;
    engine->render();

    QPainter p(this);
    auto     pal = palette();

    //! draw margins
    p.fillRect(rect(), QColor(30, 30, 30));

    //! draw text area
    p.setPen(pal.text().color());

    const auto  &fm           = engine->fm;
    const double line_spacing = engine->line_height * engine->line_spacing_ratio;
    const double y_offset     = 0.0;

    const auto flags     = Qt::AlignBaseline | Qt::TextDontClip;
    const auto text_area = textArea();
    double     y_pos     = text_area.top() + d->scroll;

    double active_block_y0 = 0.0;
    double active_block_y1 = 0.0;

    for (int i = 0; i < engine->active_blocks.size(); ++i) {
        const auto block        = engine->active_blocks[i];
        const auto block_stride = block->lines.size() * engine->line_height + engine->block_spacing;
        if (y_pos + block_stride < text_area.top() || y_pos > text_area.bottom()) {
            y_pos += block_stride;
            continue;
        }
        if (i == engine->active_block_index) { active_block_y0 = y_pos; }
        for (const auto &line : block->lines) {
            const auto text   = line.text();
            const auto incr   = line.charSpacing();
            const auto offset = line.isFirstLine() ? engine->standard_char_width * 2 : 0;
            QRectF     bb(
                text_area.left() + offset, y_pos, engine->max_width - offset, engine->line_height);
            for (auto &c : text) {
                p.drawText(bb, flags, c);
                bb.setLeft(bb.left() + incr + fm.horizontalAdvance(c));
            }
            y_pos += line_spacing;
        }
        if (i == engine->active_block_index) { active_block_y1 = y_pos; }
        y_pos += engine->block_spacing;
    }

    //! draw highlight text block
    if (engine->isCursorAvailable()) {
        QRect highlight_rect(
            text_area.left() - 8,
            active_block_y0 - engine->block_spacing,
            text_area.width() + 16,
            active_block_y1 - active_block_y0 + engine->block_spacing * 1.5);
        p.save();
        p.setBrush(QColor(255, 255, 255, 10));
        p.setPen(Qt::transparent);
        p.drawRoundedRect(highlight_rect, 4, 4);
        p.restore();
    }

    //! draw cursor
    if (engine->isCursorAvailable() && d->blink_cursor_should_paint) {
        const auto &line         = engine->currentLine();
        const auto &cursor       = engine->cursor;
        const auto  offset       = line.isFirstLine() ? engine->standard_char_width * 2 : 0;
        double      cursor_x_pos = text_area.left() + offset + line.charSpacing() * cursor.col;
        double      cursor_y_pos = text_area.top() + d->scroll;
        //! NOTE: you may question about why it doesn't call `fm.horizontalAdvance(text)` directly,
        //! and the reason is that the text_width calcualated by that has a few difference with the
        //! render result of the text, and the cursor will seems not in the correct place, and this
        //! problem was extremely serious in pure latin texts
        for (const auto &c : line.text().mid(0, cursor.col)) {
            cursor_x_pos += fm.horizontalAdvance(c);
        }
        for (int i = 0; i < engine->active_block_index; ++i) {
            cursor_y_pos +=
                line_spacing * engine->active_blocks[i]->lines.size() + engine->block_spacing;
        }
        cursor_y_pos += cursor.row * line_spacing;
        if (d->edit_op_happens) {
            if (cursor_y_pos > text_area.bottom()) {
                const double delta = cursor_y_pos + fm.height() - text_area.bottom();
                scroll(-delta - d->engine->block_spacing);
            } else if (cursor_y_pos + fm.height() < text_area.top()) {
                const double delta = text_area.top() - cursor_y_pos;
                scroll(delta + d->engine->block_spacing);
            }
        }
        p.drawLine(cursor_x_pos, cursor_y_pos, cursor_x_pos, cursor_y_pos + fm.height());
    }

    d->edit_op_happens = false;
}

void LimitedViewEditor::focusInEvent(QFocusEvent *e) {
    QWidget::focusInEvent(e);
    if (auto e = d->engine; e->isEmpty()) {
        e->setTextRefUnsafe(&d->text, 0);
        e->insertBlock(0);
        //! TODO: move it into a safe method
        e->active_block_index = 0;
    }
    postUpdateRequest();
}

void LimitedViewEditor::focusOutEvent(QFocusEvent *e) {
    QWidget::focusOutEvent(e);
    d->blink_timer.stop();
    d->engine->active_block_index = -1;
}

void LimitedViewEditor::keyPressEvent(QKeyEvent *e) {
    const auto  key    = e->key() | e->modifiers();
    const auto &cursor = d->engine->cursor;

    if (auto text = e->text(); !text.isEmpty() && text.at(0).isPrint()) {
        insert(text.at(0));
    } else if (key == Qt::Key_Return || key == Qt::Key_Enter) {
        splitIntoNewLine();
    } else if (e->matches(QKeySequence::MoveToPreviousChar)) {
        move(-1);
    } else if (e->matches(QKeySequence::MoveToNextChar)) {
        move(1);
    } else if (e->matches(QKeySequence::MoveToStartOfLine)) {
        const int offset = cursor.col == 0 ? cursor.pos : cursor.col;
        move(-offset);
    } else if (e->matches(QKeySequence::MoveToEndOfLine)) {
        const auto block  = d->engine->currentBlock();
        auto       len    = block->lengthOfLine(cursor.row);
        const int  offset = cursor.col == len ? block->textLength() - cursor.pos : len - cursor.col;
        move(offset);
    } else if (e->matches(QKeySequence::MoveToPreviousPage)) {
        scroll(textArea().height() * 0.5);
    } else if (e->matches(QKeySequence::MoveToNextPage)) {
        scroll(-textArea().height() * 0.5);
    } else if (key == QKeySequence::fromString("Ctrl+Up")) {
        scroll(d->engine->line_height * d->engine->line_spacing_ratio);
    } else if (key == QKeySequence::fromString("Ctrl+Down")) {
        scroll(-d->engine->line_height * d->engine->line_spacing_ratio);
    } else if (e->matches(QKeySequence::MoveToStartOfDocument)) {
        auto &cursor                  = d->engine->cursor;
        d->engine->active_block_index = 0;
        cursor.pos                    = 0;
        cursor.row                    = 0;
        cursor.col                    = 0;
        d->cursor_pos                 = 0;
        scrollToStart();
    } else if (e->matches(QKeySequence::MoveToEndOfDocument)) {
        auto &cursor                  = d->engine->cursor;
        d->engine->active_block_index = d->engine->active_blocks.size() - 1;
        auto block                    = d->engine->currentBlock();
        cursor.pos                    = block->textLength();
        cursor.row                    = block->lines.size() - 1;
        cursor.col                    = block->lengthOfLine(block->lines.size() - 1);
        d->cursor_pos                 = d->text.size();
        scrollToEnd();
    } else if (e->matches(QKeySequence::Copy)) {
        copy();
    } else if (e->matches(QKeySequence::Cut)) {
        cut();
    } else if (e->matches(QKeySequence::Paste)) {
        paste();
    } else if (e->matches(QKeySequence::Delete)) {
        del(1);
    } else if (key == Qt::Key_Backspace) {
        del(-1);
    } else if (key == QKeySequence::fromString("Ctrl+U")) {
        del(-qMax(d->engine->cursor.col, 1));
    } else if (key == QKeySequence::fromString("Ctrl+K")) {
        const auto block  = d->engine->currentBlock();
        const auto cursor = d->engine->cursor;
        del(qMax(block->lengthOfLine(cursor.row) - cursor.col, 1));
    } else if (e->matches(QKeySequence::MoveToPreviousLine)) {
        auto block = d->engine->currentBlock();
        if (d->engine->active_block_index == 0 && cursor.row == 0) {
            move(-cursor.pos);
        } else if (cursor.row == 0) {
            const auto prev_block  = d->engine->active_blocks[d->engine->active_block_index - 1];
            const int  equiv_col   = prev_block->lines.size() == 1 ? cursor.col : cursor.col + 2;
            const int  line_length = prev_block->lengthOfLine(prev_block->lines.size() - 1);
            const int  col         = qMin(equiv_col, line_length);
            move(-(cursor.pos + (line_length - col) + 1));
        } else {
            const int equiv_col = cursor.row - 1 == 0 ? cursor.col - 2 : cursor.col;
            const int col       = qBound(0, equiv_col, block->lengthOfLine(cursor.row - 1));
            const int pos       = block->offsetOfLine(cursor.row - 1) + col;
            move(pos - cursor.pos);
        }
    } else if (e->matches(QKeySequence::MoveToNextLine)) {
        const auto &cursor = d->engine->cursor;
        auto        block  = d->engine->currentBlock();
        if (d->engine->active_block_index + 1 == d->engine->active_blocks.size()
            && cursor.row + 1 == block->lines.size()) {
            move(block->textLength() - cursor.pos);
        } else if (cursor.row + 1 == block->lines.size()) {
            const auto next_block = d->engine->active_blocks[d->engine->active_block_index + 1];
            const int  equiv_col  = cursor.row == 0 ? cursor.col : cursor.col - 2;
            const int  col        = qBound(0, equiv_col, next_block->lengthOfLine(0));
            move(block->textLength() - cursor.pos + col + 1);
        } else {
            const int equiv_col = cursor.row == 0 ? cursor.col + 2 : cursor.col;
            const int col       = qMin(equiv_col, block->lengthOfLine(cursor.row + 1));
            const int pos       = block->offsetOfLine(cursor.row + 1) + col;
            move(pos - cursor.pos);
        }
    } else if (key == QKeySequence::fromString("Ctrl+Return")) {
        d->engine->insertBlock(d->engine->active_block_index + 1);
        auto block = d->engine->currentBlock();
        move(block->textLength() - d->engine->cursor.pos + 1);
    }

    e->accept();
}

void LimitedViewEditor::mousePressEvent(QMouseEvent *e) {
    QWidget::mousePressEvent(e);

    if (const auto e = d->engine; e->isEmpty() || e->isDirty()) { return; }
    if (d->engine->preedit) { return; }

    const auto &area = textArea();
    if (!area.contains(e->pos())) { return; }

    //! TODO: cache block bounds to accelerate location query

    QPointF      pos           = e->pos() - area.topLeft() - QPointF(0, d->scroll);
    const double line_spacing  = d->engine->line_height * d->engine->line_spacing_ratio;
    const double block_spacing = d->engine->block_spacing;

    int    block_index = 0;
    double y_pos       = 0.0;
    double y_limit     = y_pos;
    while (block_index < d->engine->active_blocks.size()) {
        auto block = d->engine->active_blocks[block_index];
        y_limit    = y_pos + block->lines.size() * line_spacing + block_spacing;
        if (pos.y() < y_limit) { break; }
        if (block_index + 1 == d->engine->active_blocks.size()) { break; }
        y_pos = y_limit;
        ++block_index;
    }

    auto        block  = d->engine->active_blocks[block_index];
    const int   row    = qBound<int>(0, (pos.y() - y_pos) / line_spacing, block->lines.size() - 1);
    const auto &line   = block->lines[row];
    const auto  text   = line.text();
    const auto  incr   = line.charSpacing();
    const auto  offset = line.isFirstLine() ? d->engine->standard_char_width * 2 : 0;

    double      x_pos = offset;
    int         col   = 0;
    const auto &fm    = d->engine->fm;
    while (col < text.length()) {
        const double char_width = fm.horizontalAdvance(text[col]) + incr;
        if (pos.x() < x_pos + 0.5 * char_width) { break; }
        x_pos += char_width;
        ++col;
    }

    auto &active_block_index = d->engine->active_block_index;
    auto &cursor             = d->engine->cursor;
    if (active_block_index == block_index && cursor.row == row && cursor.col == col) { return; }

    active_block_index = block_index;
    cursor.pos         = line.textOffset() + col;
    cursor.row         = row;
    cursor.col         = col;
    d->cursor_pos      = d->engine->currentBlock()->text_pos + cursor.pos;

    postUpdateRequest();
}

void LimitedViewEditor::mouseReleaseEvent(QMouseEvent *e) {
    QWidget::mouseReleaseEvent(e);
}

void LimitedViewEditor::mouseDoubleClickEvent(QMouseEvent *e) {
    QWidget::mouseDoubleClickEvent(e);
}

void LimitedViewEditor::mouseMoveEvent(QMouseEvent *e) {
    QWidget::mouseMoveEvent(e);
    const auto &area = textArea();
    if (area.contains(e->pos())) {
        setCursor(Qt::IBeamCursor);
    } else {
        setCursor(Qt::ArrowCursor);
    }
}

void LimitedViewEditor::wheelEvent(QWheelEvent *e) {
    const auto engine = d->engine;
    if (engine->isEmpty()) { return; }
    const double ratio = 1.0 / 180 * 3;
    const auto   delta = e->angleDelta().y() * engine->line_height * ratio;
    scroll(delta);
}

void LimitedViewEditor::inputMethodEvent(QInputMethodEvent *e) {
    if (!d->engine->isCursorAvailable()) { return; }

    const auto &cursor                = d->engine->cursor;
    const auto &saved_cursor          = d->engine->saved_cursor;
    const auto  last_preedit_text_len = cursor.pos - saved_cursor.pos;

    //! FIXME: methods for IME preedit introduce some unexpected bugs, but i can't concretely
    //! describe them so far

    if (const auto preedit_text = e->preeditString(); !preedit_text.isEmpty()) {
        if (!d->engine->preedit) {
            d->engine->beginPreEdit(d->preedit_text);
        } else {
            d->preedit_text.remove(saved_cursor.pos, last_preedit_text_len);
        }
        d->preedit_text.insert(saved_cursor.pos, preedit_text);
        d->engine->updatePreEditText(preedit_text.length());
        postUpdateRequest();
    } else {
        if (d->engine->preedit) { d->engine->commitPreEdit(); }
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
