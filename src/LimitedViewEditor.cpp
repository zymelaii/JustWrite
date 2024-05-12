#include "LimitedViewEditor.h"
#include "TextViewEngine.h"
#include "TextInputCommand.h"
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
#include <magic_enum.hpp>
#include <chrono>
#include <array>

#ifndef NDEBUG
#define ON_DEBUG(...) __VA_ARGS__
#else
#define ON_DEBUG(...)
#endif

ON_DEBUG(enum class ProfileTarget{
    IME2UpdateDelay,
    FrameRenderCost,
    TextEngineRenderCost,
    GeneralTextEdit,
};)

ON_DEBUG(struct {
    using timestamp_t     = decltype(std::chrono::system_clock::now());
    using duration_t      = std::chrono::milliseconds;
    using duration_list_t = QVector<duration_t>;
    using start_record_t  = std::array<timestamp_t, magic_enum::enum_count<ProfileTarget>()>;
    using profile_data_t  = std::array<duration_list_t, magic_enum::enum_count<ProfileTarget>()>;

    start_record_t start_record;
    profile_data_t profile_data;
    QTimer        *timer = nullptr;

    int indexof(ProfileTarget target) {
        return *magic_enum::enum_index(target);
    }

    void start(ProfileTarget target) {
        auto &rec = start_record[indexof(target)];
        if (rec.time_since_epoch().count() == 0) { rec = std::chrono::system_clock::now(); }
    }

    void record(ProfileTarget target) {
        const auto index = indexof(target);
        auto      &rec   = start_record[index];
        if (rec.time_since_epoch().count() != 0) {
            auto now = std::chrono::system_clock::now();
            profile_data[index].push_back(std::chrono::duration_cast<duration_t>(now - rec));
            rec = timestamp_t{};
        }
    }

    int total_valid() {
        int count = 0;
        for (auto data : profile_data) {
            if (!data.empty()) { ++count; }
        }
        return count;
    }

    float averageof(ProfileTarget target) {
        const auto &data = profile_data[indexof(target)];
        float       sum  = 0;
        for (auto dur : data) { sum += dur.count(); }
        return sum / data.size();
    }

    void clear(ProfileTarget target) {
        profile_data[indexof(target)].clear();
    }

    void setup(QObject * parent) {
        timer = new QTimer(parent);
        timer->setInterval(10000);
        timer->setSingleShot(false);
        timer->start();
        QObject::connect(timer, &QTimer::timeout, parent, [this]() {
            if (total_valid() == 0) { return; }
            qDebug().nospace() << "PROFILE RECORD";
            for (auto target : magic_enum::enum_values<ProfileTarget>()) {
                const auto &data = profile_data[indexof(target)];
                if (data.empty()) { continue; }
                qDebug().noquote() << QStringLiteral("  %1 %2ms")
                                          .arg(magic_enum::enum_name(target).data())
                                          .arg(averageof(target));
                clear(target);
            }
        });
    }
} Profiler;)

struct LimitedViewEditorPrivate {
    int                              min_text_line_chars;
    bool                             align_center;
    double                           scroll;
    double                           expected_scroll;
    bool                             edit_op_happens;
    bool                             blink_cursor_should_paint;
    QTimer                           blink_timer;
    int                              cursor_pos;
    QString                          text;
    QString                          preedit_text;
    jwrite::TextViewEngine          *engine;
    jwrite::TextInputCommandManager *input_manager;

    LimitedViewEditorPrivate() {
        engine                    = nullptr;
        min_text_line_chars       = 12;
        cursor_pos                = 0;
        align_center              = false;
        scroll                    = 0.0;
        expected_scroll           = scroll;
        edit_op_happens           = false;
        blink_cursor_should_paint = true;
        blink_timer.setInterval(500);
        blink_timer.setSingleShot(false);
    }

    ~LimitedViewEditorPrivate() {
        Q_ASSERT(engine);
        delete input_manager;
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
    setAutoFillBackground(true);
    setAcceptDrops(true);
    setMouseTracking(true);
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

    setAlignCenter(true);

    d->engine        = new jwrite::TextViewEngine(fontMetrics(), textArea().width());
    d->input_manager = new jwrite::TextInputCommandManager(*d->engine);
    d->input_manager->loadDefaultMappings();

    scrollToStart();

    connect(this, &LimitedViewEditor::textAreaChanged, this, [this](QRect area) {
        d->engine->resetMaxWidth(area.width());
        update();
    });
    connect(&d->blink_timer, &QTimer::timeout, this, [this] {
        d->blink_cursor_should_paint = !d->blink_cursor_should_paint;
        update();
    });

    ON_DEBUG(Profiler.setup(this));
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

void LimitedViewEditor::scroll(double delta, bool smooth) {
    const auto e            = d->engine;
    const auto line_spacing = e->line_spacing_ratio * e->line_height;
    double     max_scroll   = line_spacing + e->block_spacing;
    double     min_scroll   = 0;
    for (auto block : e->active_blocks) {
        min_scroll -= block->lines.size() * line_spacing + e->block_spacing;
    }
    min_scroll         += line_spacing + e->block_spacing;
    d->expected_scroll  = qBound<double>(min_scroll, d->scroll + delta, max_scroll);
    if (!smooth) { d->scroll = d->expected_scroll; }
    update();
}

void LimitedViewEditor::scrollToStart() {
    const auto   e            = d->engine;
    const auto   line_spacing = e->line_spacing_ratio * e->line_height;
    const double max_scroll   = line_spacing + e->block_spacing;
    d->expected_scroll        = max_scroll;
    d->scroll                 = d->expected_scroll;
    postUpdateRequest();
}

void LimitedViewEditor::scrollToEnd() {
    const auto e            = d->engine;
    const auto line_spacing = e->line_spacing_ratio * e->line_height;
    double     min_scroll   = 0;
    for (auto block : e->active_blocks) {
        min_scroll -= block->lines.size() * line_spacing + e->block_spacing;
    }
    min_scroll         += line_spacing + e->block_spacing;
    d->expected_scroll  = min_scroll;
    d->scroll           = d->expected_scroll;
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

    emit textChanged(*d->engine->text_ref);
    postUpdateRequest();
}

void LimitedViewEditor::del(int times) {
    //! NOTE: times means delete |times| chars, and the sign indicates the del direction
    //! TODO: delete selected text
    int       deleted  = 0;
    const int offset   = d->engine->commitDeletion(times, deleted);
    d->cursor_pos     += offset;
    d->text.remove(d->cursor_pos, deleted);

    emit textChanged(*d->engine->text_ref);
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

LimitedViewEditor::TextLoc LimitedViewEditor::getTextLocAtPos(const QPoint &pos) {
    TextLoc loc{.block_index = -1, .row = 0, .col = 0};

    const auto &area = textArea();
    if (!area.contains(pos)) { return loc; }

    //! TODO: cache block bounds to accelerate location query

    QPointF      real_pos      = pos - area.topLeft() - QPointF(0, d->scroll);
    const double line_spacing  = d->engine->line_height * d->engine->line_spacing_ratio;
    const double block_spacing = d->engine->block_spacing;

    int    block_index = 0;
    double y_pos       = 0.0;
    double y_limit     = y_pos;
    while (block_index < d->engine->active_blocks.size()) {
        auto block = d->engine->active_blocks[block_index];
        y_limit    = y_pos + block->lines.size() * line_spacing + block_spacing;
        if (real_pos.y() < y_limit) { break; }
        if (block_index + 1 == d->engine->active_blocks.size()) { break; }
        y_pos = y_limit;
        ++block_index;
    }

    auto      block = d->engine->active_blocks[block_index];
    const int row = qBound<int>(0, (real_pos.y() - y_pos) / line_spacing, block->lines.size() - 1);
    const auto &line   = block->lines[row];
    const auto  text   = line.text();
    const auto  incr   = line.charSpacing();
    const auto  offset = line.isFirstLine() ? d->engine->standard_char_width * 2 : 0;

    double      x_pos = offset;
    int         col   = 0;
    const auto &fm    = d->engine->fm;
    while (col < text.length()) {
        const double char_width = fm.horizontalAdvance(text[col]) + incr;
        if (real_pos.x() < x_pos + 0.5 * char_width) { break; }
        x_pos += char_width;
        ++col;
    }

    loc.block_index = block_index;
    loc.row         = row;
    loc.col         = col;

    return loc;
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
    ON_DEBUG(Profiler.start(ProfileTarget::FrameRenderCost));

    ON_DEBUG(Profiler.start(ProfileTarget::TextEngineRenderCost));
    auto engine = d->engine;
    engine->render();
    ON_DEBUG(Profiler.record(ProfileTarget::TextEngineRenderCost));

    //! smooth scroll
    d->scroll = d->scroll * 0.45 + d->expected_scroll * 0.55;
    if (qAbs(d->scroll - d->expected_scroll) > 1e-3) {
        update();
    } else {
        d->scroll = d->expected_scroll;
    }

    QPainter p(this);
    auto     pal = palette();

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
            const double margin = engine->block_spacing * 0.75;
            if (auto bottom = cursor_y_pos + fm.height() + margin; bottom > text_area.bottom()) {
                const double delta = bottom - text_area.bottom();
                scroll(-delta - d->engine->block_spacing, false);
            } else if (auto top = cursor_y_pos - margin; top < text_area.top()) {
                const double delta = text_area.top() - top;
                scroll(delta + d->engine->block_spacing, false);
            }
        }
        p.drawLine(cursor_x_pos, cursor_y_pos, cursor_x_pos, cursor_y_pos + fm.height());
    }

    d->edit_op_happens = false;

    ON_DEBUG(Profiler.record(ProfileTarget::IME2UpdateDelay));
    ON_DEBUG(Profiler.record(ProfileTarget::FrameRenderCost));
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
    emit focusLost();
}

void LimitedViewEditor::keyPressEvent(QKeyEvent *e) {
    if (!d->engine->isCursorAvailable()) { return; }

    const auto action = d->input_manager->match(e);
    qDebug() << "COMMAND" << magic_enum::enum_name(action).data();

    const auto &cursor = d->engine->cursor;

    ON_DEBUG(Profiler.start(ProfileTarget::GeneralTextEdit));

    switch (action) {
        case TextInputCommand::Reject: {
        } break;
        case TextInputCommand::InsertPrintable: {
            insert(d->input_manager->translatePrintableChar(e));
        } break;
        case TextInputCommand::InsertTab: {
        } break;
        case TextInputCommand::InsertNewLine: {
            splitIntoNewLine();
        } break;
        case TextInputCommand::Cancel: {
        } break;
        case TextInputCommand::Undo: {
        } break;
        case TextInputCommand::Redo: {
        } break;
        case TextInputCommand::Copy: {
            copy();
        } break;
        case TextInputCommand::Cut: {
            cut();
        } break;
        case TextInputCommand::Paste: {
            paste();
        } break;
        case TextInputCommand::ScrollUp: {
            scroll(d->engine->line_height * d->engine->line_spacing_ratio, true);
        } break;
        case TextInputCommand::ScrollDown: {
            scroll(-d->engine->line_height * d->engine->line_spacing_ratio, true);
        } break;
        case TextInputCommand::MoveToPrevChar: {
            move(-1);
        } break;
        case TextInputCommand::MoveToNextChar: {
            move(1);
        } break;
        case TextInputCommand::MoveToPrevWord: {
        } break;
        case TextInputCommand::MoveToNextWord: {
        } break;
        case TextInputCommand::MoveToPrevLine: {
            auto block = d->engine->currentBlock();
            if (d->engine->active_block_index == 0 && cursor.row == 0) {
                move(-cursor.pos);
            } else if (cursor.row == 0) {
                const auto prev_block = d->engine->active_blocks[d->engine->active_block_index - 1];
                const int  equiv_col  = prev_block->lines.size() == 1 ? cursor.col : cursor.col + 2;
                const int  line_length = prev_block->lengthOfLine(prev_block->lines.size() - 1);
                const int  col         = qMin(equiv_col, line_length);
                move(-(cursor.pos + (line_length - col) + 1));
            } else {
                const int equiv_col = cursor.row - 1 == 0 ? cursor.col - 2 : cursor.col;
                const int col       = qBound(0, equiv_col, block->lengthOfLine(cursor.row - 1));
                const int pos       = block->offsetOfLine(cursor.row - 1) + col;
                move(pos - cursor.pos);
            }
        } break;
        case TextInputCommand::MoveToNextLine: {
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
        } break;
        case TextInputCommand::MoveToStartOfLine: {
            move(-cursor.col);
        } break;
        case TextInputCommand::MoveToEndOfLine: {
            move(d->engine->currentBlock()->lengthOfLine(cursor.row) - cursor.col);
        } break;
        case TextInputCommand::MoveToStartOfBlock: {
            move(-cursor.pos);
        } break;
        case TextInputCommand::MoveToEndOfBlock: {
            move(d->engine->currentBlock()->textLength() - cursor.pos);
        } break;
        case TextInputCommand::MoveToStartOfDocument: {
            d->engine->active_block_index = 0;
            d->engine->cursor.reset();
            d->cursor_pos = 0;
            scrollToStart();
        } break;
        case TextInputCommand::MoveToEndOfDocument: {
            auto &cursor                  = d->engine->cursor;
            d->engine->active_block_index = d->engine->active_blocks.size() - 1;
            auto block                    = d->engine->currentBlock();
            cursor.pos                    = block->textLength();
            cursor.row                    = block->lines.size() - 1;
            cursor.col                    = block->lengthOfLine(block->lines.size() - 1);
            d->cursor_pos                 = d->text.size();
            scrollToEnd();
        } break;
        case TextInputCommand::MoveToPrevPage: {
            scroll(textArea().height() * 0.5, true);
        } break;
        case TextInputCommand::MoveToNextPage: {
            scroll(-textArea().height() * 0.5, true);
        } break;
        case TextInputCommand::MoveToPrevBlock: {
            if (d->engine->active_block_index > 0) {
                --d->engine->active_block_index;
                d->engine->cursor.reset();
                d->cursor_pos = d->engine->currentBlock()->text_pos;
                postUpdateRequest();
            }
        } break;
        case TextInputCommand::MoveToNextBlock: {
            if (d->engine->active_block_index + 1 < d->engine->active_blocks.size()) {
                ++d->engine->active_block_index;
                d->engine->cursor.reset();
                d->cursor_pos = d->engine->currentBlock()->text_pos;
                postUpdateRequest();
            }
        } break;
        case TextInputCommand::DeletePrevChar: {
            del(-1);
        } break;
        case TextInputCommand::DeleteNextChar: {
            del(1);
        } break;
        case TextInputCommand::DeletePrevWord: {
        } break;
        case TextInputCommand::DeleteNextWord: {
        } break;
        case TextInputCommand::DeleteToStartOfLine: {
            del(-qMax(d->engine->cursor.col, 1));
        } break;
        case TextInputCommand::DeleteToEndOfLine: {
            const auto block  = d->engine->currentBlock();
            const auto cursor = d->engine->cursor;
            del(qMax(block->lengthOfLine(cursor.row) - cursor.col, 1));
        } break;
        case TextInputCommand::DeleteToStartOfBlock: {
        } break;
        case TextInputCommand::DeleteToEndOfBlock: {
        } break;
        case TextInputCommand::DeleteToStartOfDocument: {
        } break;
        case TextInputCommand::DeleteToEndOfDocument: {
        } break;
        case TextInputCommand::SelectPrevChar: {
        } break;
        case TextInputCommand::SelectNextChar: {
        } break;
        case TextInputCommand::SelectPrevWord: {
        } break;
        case TextInputCommand::SelectNextWord: {
        } break;
        case TextInputCommand::SelectToStartOfLine: {
        } break;
        case TextInputCommand::SelectToEndOfLine: {
        } break;
        case TextInputCommand::SelectToStartOfBlock: {
        } break;
        case TextInputCommand::SelectToEndOfBlock: {
        } break;
        case TextInputCommand::SelectBlock: {
        } break;
        case TextInputCommand::SelectPrevPage: {
        } break;
        case TextInputCommand::SelectNextPage: {
        } break;
        case TextInputCommand::SelectToStartOfDoc: {
        } break;
        case TextInputCommand::SelectToEndOfDoc: {
        } break;
        case TextInputCommand::SelectAll: {
        } break;
        case TextInputCommand::InsertBeforeBlock: {
            d->engine->insertBlock(d->engine->active_block_index);
            --d->engine->active_block_index;
            d->engine->cursor.reset();
            d->cursor_pos = d->engine->currentBlock()->text_pos;
            postUpdateRequest();
        } break;
        case TextInputCommand::InsertAfterBlock: {
            d->engine->insertBlock(d->engine->active_block_index + 1);
            auto block = d->engine->currentBlock();
            move(block->textLength() - d->engine->cursor.pos + 1);
        } break;
    }

    ON_DEBUG(Profiler.record(ProfileTarget::GeneralTextEdit));

    e->accept();
}

void LimitedViewEditor::mousePressEvent(QMouseEvent *e) {
    QWidget::mousePressEvent(e);

    if (const auto e = d->engine; e->isEmpty() || e->isDirty()) { return; }
    if (d->engine->preedit) { return; }

    const auto loc = getTextLocAtPos(e->pos());
    if (loc.block_index == -1) { return; }

    auto &active_block_index = d->engine->active_block_index;
    auto &cursor             = d->engine->cursor;
    if (active_block_index == loc.block_index && cursor.row == loc.row && cursor.col == loc.col) {
        return;
    }

    active_block_index = loc.block_index;
    cursor.row         = loc.row;
    cursor.col         = loc.col;
    cursor.pos         = d->engine->currentLine().textOffset() + cursor.col;
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
    scroll(delta, true);
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
        ON_DEBUG(Profiler.start(ProfileTarget::IME2UpdateDelay));
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
