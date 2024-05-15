#include "LimitedViewEditor.h"
#include "TextViewEngine.h"
#include "TextInputCommand.h"
#include "ProfileUtils.h"
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

struct LimitedViewEditorPrivate {
    int                              min_text_line_chars;
    bool                             align_center;
    double                           scroll;
    double                           expected_scroll;
    bool                             edit_op_happens;
    bool                             blink_cursor_should_paint;
    QTimer                           blink_timer;
    QTimer                           auto_scroll_timer;
    int                              cursor_pos;
    QString                          text;
    QString                          preedit_text;
    int                              selected_from;
    int                              selected_to;
    QMargins                         margins;
    jwrite::TextViewEngine          *engine;
    jwrite::TextInputCommandManager *input_manager;
    QMap<int, double>                cached_block_y_pos;

    bool   auto_scroll_mode;
    double scroll_base_y_pos;
    double scroll_ref_y_pos;

    Qt::CursorShape cursor_shape[2];

    LimitedViewEditorPrivate() {
        engine                    = nullptr;
        min_text_line_chars       = 12;
        cursor_pos                = 0;
        align_center              = false;
        scroll                    = 0.0;
        expected_scroll           = scroll;
        edit_op_happens           = false;
        blink_cursor_should_paint = true;
        selected_from             = -1;
        selected_to               = -1;
        cursor_shape[0]           = Qt::ArrowCursor;
        cursor_shape[1]           = Qt::ArrowCursor;
        blink_timer.setInterval(500);
        blink_timer.setSingleShot(false);
        auto_scroll_timer.setInterval(10);
        auto_scroll_timer.setSingleShot(false);
    }

    ~LimitedViewEditorPrivate() {
        Q_ASSERT(engine);
        delete input_manager;
        delete engine;
    }
};

LimitedViewEditor::LimitedViewEditor(QWidget *parent)
    : QWidget(parent)
    , d{new LimitedViewEditorPrivate} {
    setFocusPolicy(Qt::ClickFocus);
    setAttribute(Qt::WA_InputMethodEnabled);
    setAutoFillBackground(true);
    setAcceptDrops(true);
    setMouseTracking(true);
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    setContentsMargins({});

    set_soft_center_mode(true);

    d->engine        = new jwrite::TextViewEngine(fontMetrics(), get_text_area().width());
    d->input_manager = new jwrite::TextInputCommandManager(*d->engine);
    d->input_manager->load_default();

    scoll_to_start();

    connect(this, &LimitedViewEditor::textAreaChanged, this, [this](QRect area) {
        d->engine->reset_max_width(area.width());
        update();
    });
    connect(&d->blink_timer, &QTimer::timeout, this, [this] {
        d->blink_cursor_should_paint = !d->blink_cursor_should_paint;
        update();
    });
    connect(&d->auto_scroll_timer, &QTimer::timeout, this, [this] {
        scroll((d->scroll_base_y_pos - d->scroll_ref_y_pos) / 10, false);
        update();
    });
}

LimitedViewEditor::~LimitedViewEditor() {
    delete d;
}

QSize LimitedViewEditor::minimumSizeHint() const {
    const auto margins      = contentsMargins() + d->margins;
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

bool LimitedViewEditor::is_soft_center_mode() const {
    return d->align_center;
}

void LimitedViewEditor::set_soft_center_mode(bool value) {
    d->align_center = value;
    if (d->align_center) {
        const auto min_margin     = 32;
        const auto max_text_width = 1000;
        const auto mean_width     = qMax(0, width() - min_margin * 2);
        const auto text_width     = qMin<int>(mean_width * 0.8, max_text_width);
        const auto margin         = (width() - text_width) / 2;
        d->margins                = QMargins(margin, 4, margin, 4);
    } else {
        d->margins = QMargins(4, 4, 4, 4);
    }
    emit textAreaChanged(get_text_area());
}

QRect LimitedViewEditor::get_text_area() const {
    return contentsRect() - d->margins;
}

EditorTextLoc LimitedViewEditor::get_current_text_loc() const {
    EditorTextLoc loc{};
    loc.block_index = d->engine->active_block_index;
    loc.pos         = d->engine->cursor.pos;
    return loc;
}

EditorTextLoc LimitedViewEditor::get_text_loc_at_pos(int pos) const {
    EditorTextLoc loc{};
    loc.block_index = -1;
    for (int i = 0; i < d->engine->active_blocks.size(); ++i) {
        auto block = d->engine->active_blocks[i];
        if (pos < block->text_pos) { continue; }
        if (pos > block->text_pos + block->text_len()) { continue; }
        int relpos = pos - block->text_pos;
        for (int j = 0; j < block->lines.size(); ++j) {
            auto line = block->lines[j];
            if (relpos > line.text_offset() + line.text().length()) { continue; }
            loc.block_index = i;
            loc.pos         = relpos;
            loc.row         = j;
            loc.col         = relpos - line.text_offset();
            return loc;
        }
    }
    return loc;
}

void LimitedViewEditor::update_text_loc(EditorTextLoc loc) {
    const auto e = d->engine;
    if (!(loc.block_index >= 0 && loc.block_index < e->active_blocks.size())) { return; }
    e->active_block_index = loc.block_index;
    e->cursor.pos         = loc.pos;
    d->cursor_pos         = e->current_block()->text_pos + e->cursor.pos;
    e->sync_cursor_row_col(0);
    scroll_to_cursor();
}

void LimitedViewEditor::reset(QString &text, bool swap) {
    if (d->engine->preedit) { cancel_preedit(); }

    QStringList blocks{};
    for (auto block : d->engine->active_blocks) { blocks << block->text().toString(); }
    auto text_out = blocks.join("\n");
    d->text.clear();

    d->engine->clear_all();
    d->engine->insert_block(0);
    d->engine->active_block_index = 0;
    d->cursor_pos                 = 0;
    d->engine->cursor.reset();
    insert_raw_text(text);

    d->engine->active_block_index = 0;
    d->cursor_pos                 = 0;
    d->engine->cursor.reset();

    if (swap) { text.swap(text_out); }

    request_update();
}

void LimitedViewEditor::cancel_preedit() {
    Q_ASSERT(d->engine->preedit);
    const auto &cursor                = d->engine->cursor;
    const auto &saved_cursor          = d->engine->saved_cursor;
    const auto  last_preedit_text_len = cursor.pos - saved_cursor.pos;
    d->preedit_text.remove(saved_cursor.pos, last_preedit_text_len);
    d->engine->update_preedit_text(0);
    d->engine->commit_preedit();
}

void LimitedViewEditor::scroll_to_cursor() {
    d->edit_op_happens = true;
    request_update();
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

void LimitedViewEditor::scoll_to_start() {
    const auto   e            = d->engine;
    const auto   line_spacing = e->line_spacing_ratio * e->line_height;
    const double max_scroll   = line_spacing + e->block_spacing;
    d->expected_scroll        = max_scroll;
    d->scroll                 = d->expected_scroll;
    request_update();
}

void LimitedViewEditor::scroll_to_end() {
    const auto e            = d->engine;
    const auto line_spacing = e->line_spacing_ratio * e->line_height;
    double     min_scroll   = 0;
    for (auto block : e->active_blocks) {
        min_scroll -= block->lines.size() * line_spacing + e->block_spacing;
    }
    min_scroll         += line_spacing + e->block_spacing;
    d->expected_scroll  = min_scroll;
    d->scroll           = d->expected_scroll;
    request_update();
}

void LimitedViewEditor::insert_raw_text(const QString &text) {
    auto text_list = text.split('\n');
    for (int i = 0; i < text_list.size(); ++i) {
        auto text = text_list[i].trimmed();
        if (text.isEmpty()) { continue; }
        insert(text);
        if (i + 1 < text_list.size()) { break_into_newline(); }
    }
}

bool LimitedViewEditor::inserted_pair_filter(const QString &text) {
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
        move(-1, false);
        return true;
    }

    if (auto index = QUOTE_PAIRS.values().indexOf(text); index != -1) {
        int pos = d->engine->current_block()->text_pos + d->engine->cursor.pos;
        if (pos == d->engine->text_ref->length()) { return false; }
        if (d->engine->text_ref->at(pos) == text) { return true; }
    }

    return false;
}

void LimitedViewEditor::move(int offset, bool extend_sel) {
    if (offset == 0) { return; }

    bool cursor_moved = false;
    int  text_offset  = 0;

    //! FIXME: cursor within selected region
    if (has_sel() && !extend_sel) {
        const int sel_from   = qMin(d->selected_from, d->selected_to);
        const int sel_to     = qMax(d->selected_from, d->selected_to);
        const int target_pos = offset > 0 ? sel_to : sel_from;
        text_offset  = d->engine->commit_movement(target_pos - d->cursor_pos, &cursor_moved, true);
        offset      -= offset > 0 ? 1 : -1;
    }

    bool cursor_moved_tmp  = cursor_moved;
    text_offset           += d->engine->commit_movement(offset, &cursor_moved, false);
    cursor_moved           = cursor_moved || cursor_moved_tmp;

    if (!extend_sel) {
        unset_sel();
    } else if (has_sel()) {
        Q_ASSERT(
            d->cursor_pos >= d->selected_from && d->cursor_pos <= d->selected_to
            || d->cursor_pos >= d->selected_to && d->cursor_pos <= d->selected_from);
        d->selected_to += text_offset;
    } else {
        d->selected_from = d->cursor_pos;
        d->selected_to   = d->cursor_pos + text_offset;
    }

    d->cursor_pos += text_offset;
    if (cursor_moved) { request_update(); }
}

void LimitedViewEditor::move_to(int pos, bool extend_sel) {
    //! ATTENTION: pos equals cursor_pos doesn't mean that no action will be taken

    if (!has_sel() && extend_sel) { d->selected_from = d->cursor_pos; }

    d->cursor_pos += d->engine->commit_movement(pos - d->cursor_pos, nullptr, true);

    if (extend_sel) {
        d->selected_to = d->cursor_pos;
    } else if (has_sel()) {
        unset_sel();
    }

    request_update();
}

void LimitedViewEditor::insert(const QString &text) {
    Q_ASSERT(d->engine->is_cursor_available());

    if (inserted_pair_filter(text)) { return; }

    d->text.insert(d->cursor_pos, text);
    const int len  = text.length();
    d->cursor_pos += len;
    d->engine->commit_insertion(len);

    emit textChanged(*d->engine->text_ref);
    request_update();
}

void LimitedViewEditor::del(int times) {
    //! NOTE: times means delete |times| chars, and the sign indicates the del direction
    //! TODO: delete selected text
    int       deleted  = 0;
    const int offset   = d->engine->commit_deletion(times, deleted);
    d->cursor_pos     += offset;
    d->text.remove(d->cursor_pos, deleted);

    emit textChanged(*d->engine->text_ref);
    request_update();
}

void LimitedViewEditor::copy() {
    if (!d->engine->is_cursor_available()) { return; }
    auto clipboard = QGuiApplication::clipboard();
    if (has_sel()) {
        //! TODO: copy selected text
    } else {
        auto block_text = d->engine->current_block()->text().toString();
        clipboard->setText(block_text);
    }
}

void LimitedViewEditor::cut() {
    if (has_sel()) {
        //! TODO: cut selected text
    } else {
        copy();
        move(-d->engine->cursor.pos, false);
        del(d->engine->current_block()->text_len());
    }
}

void LimitedViewEditor::paste() {
    auto clipboard = QGuiApplication::clipboard();
    auto mime      = clipboard->mimeData();
    if (!mime->hasText()) { return; }
    insert_raw_text(clipboard->text());
}

bool LimitedViewEditor::has_sel() const {
    return d->selected_from != -1 && d->selected_to != -1 && d->selected_from != d->selected_to;
}

void LimitedViewEditor::unset_sel() {
    if (has_sel()) {
        d->selected_from = -1;
        d->selected_to   = -1;
        request_update();
    }
}

void LimitedViewEditor::break_into_newline() {
    d->engine->break_block_at_cursor_pos();
    request_update();
}

EditorTextLoc LimitedViewEditor::get_text_loc_at_vpos(const QPoint &pos) {
    EditorTextLoc loc{.block_index = -1, .row = 0, .col = 0};

    const auto &area = get_text_area();
    if (!area.contains(pos)) { return loc; }

    //! TODO: cache block bounds to accelerate location query

    jwrite_profiler_start(GetLocationAtPos);

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
    const auto  incr   = line.char_spacing();
    const auto  offset = line.is_first_line() ? d->engine->standard_char_width * 2 : 0;

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

    jwrite_profiler_record(GetLocationAtPos);

    return loc;
}

void LimitedViewEditor::request_update() {
    d->blink_cursor_should_paint = true;
    d->edit_op_happens           = true;
    d->blink_timer.stop();
    update();
    d->blink_timer.start();
}

void LimitedViewEditor::set_cursor_shape(Qt::CursorShape shape) {
    d->cursor_shape[1] = d->cursor_shape[0];
    d->cursor_shape[0] = shape;
    setCursor(shape);
}

void LimitedViewEditor::restore_cursor_shape() {
    set_cursor_shape(d->cursor_shape[1]);
}

void LimitedViewEditor::resizeEvent(QResizeEvent *e) {
    QWidget::resizeEvent(e);
    if (d->align_center) { set_soft_center_mode(true); }
    emit textAreaChanged(get_text_area());
}

void LimitedViewEditor::paintEvent(QPaintEvent *e) {
    jwrite_profiler_start(FrameRenderCost);

    jwrite_profiler_start(TextEngineRenderCost);
    auto engine = d->engine;
    engine->render();
    jwrite_profiler_record(TextEngineRenderCost);

    QPainter p(this);
    auto     pal = palette();

    struct {
        const QColor Selection = QColor(60, 60, 255, 80);
        const QColor Highlight = QColor(255, 255, 255, 10);
    } Palette;

    //! smooth scroll
    d->scroll = d->scroll * 0.45 + d->expected_scroll * 0.55;
    if (qAbs(d->scroll - d->expected_scroll) > 1e-3) {
        update();
    } else {
        d->scroll = d->expected_scroll;
    }

    //! prepare render data
    jwrite_profiler_start(PrepareRenderData);

    const auto  &fm           = engine->fm;
    const double line_spacing = engine->line_height * engine->line_spacing_ratio;
    const auto   text_area    = get_text_area();
    double       y_pos        = text_area.top() + d->scroll;

    struct {
        struct {
            double first_non_clip_block;
            double active_block_begin;
            double active_block_end;
        } named_y_pos;

        struct BlockRange {
            int first;
            int last;
        };

        BlockRange visible_range;
        BlockRange select_range;

        bool found_non_clip_block;
    } context;

    context.visible_range        = {-1, -1};
    context.found_non_clip_block = false;

    d->cached_block_y_pos.clear();

    for (int index = 0; index < engine->active_blocks.size(); ++index) {
        const auto block        = engine->active_blocks[index];
        const auto block_stride = block->lines.size() * engine->line_height + engine->block_spacing;
        if (y_pos + block_stride < text_area.top()) {
            y_pos += block_stride;
            continue;
        } else if (y_pos > text_area.bottom()) {
            break;
        } else if (!context.found_non_clip_block) {
            context.named_y_pos.first_non_clip_block = y_pos;
            context.found_non_clip_block             = true;
        }
        d->cached_block_y_pos[index] = y_pos;
        if (context.visible_range.first == -1) { context.visible_range.first = index; }
        context.visible_range.last = index;
        if (index == engine->active_block_index) { context.named_y_pos.active_block_begin = y_pos; }
        y_pos += line_spacing * block->lines.size();
        if (index == engine->active_block_index) { context.named_y_pos.active_block_end = y_pos; }
        y_pos += engine->block_spacing;
    }

    jwrite_profiler_record(PrepareRenderData);

    //! draw selection
    if (has_sel()) {
        jwrite_profiler_start(SelectionAreaRenderCost);

        auto loc_from = get_text_loc_at_pos(d->selected_from);
        auto loc_to   = get_text_loc_at_pos(d->selected_to);
        if (d->selected_from > d->selected_to) { std::swap(loc_from, loc_to); }

        context.select_range.first = qMax(loc_from.block_index, context.visible_range.first);
        context.select_range.last  = qMin(loc_to.block_index, context.visible_range.last);

        //! add the minus part to make the selection area align center
        double y_pos = context.named_y_pos.first_non_clip_block - fm.descent() * 0.5;
        for (int index = context.visible_range.first; index < context.select_range.first; ++index) {
            const auto block = engine->active_blocks[index];
            const auto block_stride =
                block->lines.size() * engine->line_height + engine->block_spacing;
            y_pos += block_stride;
        }

        for (int index = context.select_range.first; index <= context.select_range.last; ++index) {
            const auto block = engine->active_blocks[index];
            const auto block_stride =
                block->lines.size() * engine->line_height + engine->block_spacing;
            if (index < context.select_range.first) {
                y_pos += block_stride;
                continue;
            }
            if (index > context.select_range.last) { break; }
            const int line_nr_begin = index == loc_from.block_index ? loc_from.row : 0;
            const int line_nr_end   = index == loc_to.block_index
                                        ? loc_to.row
                                        : engine->active_blocks[index]->lines.size() - 1;
            auto      sel_y_pos     = y_pos + line_nr_begin * engine->line_height;
            for (int line_nr = line_nr_begin; line_nr <= line_nr_end; ++line_nr) {
                const auto line   = block->lines[line_nr];
                const auto offset = line.is_first_line() ? engine->standard_char_width * 2 : 0;
                const auto sel_x_origin = text_area.left() + offset;
                double     sel_x_pos    = sel_x_origin;
                if (index == loc_from.block_index && line_nr == loc_from.row) {
                    sel_x_pos += line.char_spacing() * loc_from.col;
                    for (const auto &c : line.text().mid(0, loc_from.col)) {
                        sel_x_pos += fm.horizontalAdvance(c);
                    }
                }
                double sel_x_width = line.cached_text_width
                                   + (line.text().length() - 1) * line.char_spacing()
                                   - (sel_x_pos - sel_x_origin);
                if (index == loc_to.block_index && line_nr == loc_to.row) {
                    sel_x_width -= line.char_spacing() * (line.text().length() - loc_to.col);
                    for (const auto &c : line.text().mid(loc_to.col)) {
                        sel_x_width -= fm.horizontalAdvance(c);
                    }
                }
                QRectF bb(sel_x_pos, sel_y_pos, sel_x_width, engine->line_height);
                p.fillRect(bb, Palette.Selection);
                sel_y_pos += engine->line_height;
            }
            y_pos += block_stride;
        }

        jwrite_profiler_record(SelectionAreaRenderCost);
    }

    //! draw highlight text block
    if (engine->is_cursor_available() && !has_sel()) {
        QRect highlight_rect(
            text_area.left() - 8,
            context.named_y_pos.active_block_begin - engine->block_spacing,
            text_area.width() + 16,
            context.named_y_pos.active_block_end - context.named_y_pos.active_block_begin
                + engine->block_spacing * 1.5);
        p.save();
        p.setBrush(Palette.Highlight);
        p.setPen(Qt::transparent);
        p.drawRoundedRect(highlight_rect, 4, 4);
        p.restore();
    }

    //! draw text area
    if (context.found_non_clip_block) {
        jwrite_profiler_start(TextBodyRenderCost);
        p.save();
        p.setPen(pal.text().color());
        const auto flags = Qt::AlignBaseline | Qt::TextDontClip;
        double     y_pos = context.named_y_pos.first_non_clip_block;
        for (int index = context.visible_range.first; index <= context.visible_range.last;
             ++index) {
            const auto block = engine->active_blocks[index];
            for (const auto &line : block->lines) {
                const auto text   = line.text();
                const auto incr   = line.char_spacing();
                const auto offset = line.is_first_line() ? engine->standard_char_width * 2 : 0;
                QRectF     bb(
                    text_area.left() + offset,
                    y_pos,
                    engine->max_width - offset,
                    engine->line_height);
                for (auto &c : text) {
                    p.drawText(bb, flags, c);
                    bb.setLeft(bb.left() + incr + fm.horizontalAdvance(c));
                }
                y_pos += line_spacing;
            }
            y_pos += engine->block_spacing;
        }
        p.restore();
        jwrite_profiler_record(TextBodyRenderCost);
    }

    //! draw cursor
    if (engine->is_cursor_available() && d->blink_cursor_should_paint) {
        jwrite_profiler_start(CursorRenderCost);
        const auto &line         = engine->current_line();
        const auto &cursor       = engine->cursor;
        const auto  offset       = line.is_first_line() ? engine->standard_char_width * 2 : 0;
        double      cursor_x_pos = text_area.left() + offset + line.char_spacing() * cursor.col;
        double      cursor_y_pos = text_area.top() + d->scroll;
        //! NOTE: you may question about why it doesn't call `fm.horizontalAdvance(text)`
        //! directly, and the reason is that the text_width calcualated by that has a few
        //! difference with the render result of the text, and the cursor will seems not in the
        //! correct place, and this problem was extremely serious in pure latin texts
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
        jwrite_profiler_record(CursorRenderCost);
    }

    d->edit_op_happens = false;

    jwrite_profiler_record(IME2UpdateDelay);
    jwrite_profiler_record(FrameRenderCost);
}

void LimitedViewEditor::focusInEvent(QFocusEvent *e) {
    QWidget::focusInEvent(e);
    if (auto e = d->engine; e->is_empty()) {
        e->set_text_ref_unsafe(&d->text, 0);
        e->insert_block(0);
        //! TODO: move it into a safe method
        e->active_block_index = 0;
        emit requireEmptyChapter();
    }
    request_update();
}

void LimitedViewEditor::focusOutEvent(QFocusEvent *e) {
    QWidget::focusOutEvent(e);
    unset_sel();
    d->blink_timer.stop();
    d->engine->active_block_index = -1;
    emit focusLost();
}

void LimitedViewEditor::keyPressEvent(QKeyEvent *e) {
    if (!d->engine->is_cursor_available()) { return; }

    //! ATTENTION: normally this branch is unreachable, but when IME events are too frequent and
    //! the system latency is too high, an IME preedit interrupt may occur and the key is
    //! forwarded to the keyPress event. in this case, we should reject the event or submit the
    //! raw preedit text. the first solution is taken here.
    //! FIXME: the solution is not fully tested to be safe and correct
    if (d->engine->preedit) { return; }

    const auto action = d->input_manager->match(e);
    qDebug() << "COMMAND" << magic_enum::enum_name(action).data();

    const auto &cursor = d->engine->cursor;

    jwrite_profiler_start(GeneralTextEdit);

    switch (action) {
        case TextInputCommand::Reject: {
        } break;
        case TextInputCommand::InsertPrintable: {
            insert(d->input_manager->translate_printable_char(e));
        } break;
        case TextInputCommand::InsertTab: {
        } break;
        case TextInputCommand::InsertNewLine: {
            break_into_newline();
        } break;
        case TextInputCommand::Cancel: {
            unset_sel();
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
            move(-1, false);
        } break;
        case TextInputCommand::MoveToNextChar: {
            move(1, false);
        } break;
        case TextInputCommand::MoveToPrevWord: {
        } break;
        case TextInputCommand::MoveToNextWord: {
        } break;
        case TextInputCommand::MoveToPrevLine: {
            auto block = d->engine->current_block();
            if (d->engine->active_block_index == 0 && cursor.row == 0) {
                move(-cursor.pos, false);
            } else if (cursor.row == 0) {
                const auto prev_block = d->engine->active_blocks[d->engine->active_block_index - 1];
                const int  equiv_col  = prev_block->lines.size() == 1 ? cursor.col : cursor.col + 2;
                const int  line_length = prev_block->len_of_line(prev_block->lines.size() - 1);
                const int  col         = qMin(equiv_col, line_length);
                move(-(cursor.pos + (line_length - col) + 1), false);
            } else {
                const int equiv_col = cursor.row - 1 == 0 ? cursor.col - 2 : cursor.col;
                const int col       = qBound(0, equiv_col, block->len_of_line(cursor.row - 1));
                const int pos       = block->offset_of_line(cursor.row - 1) + col;
                move(pos - cursor.pos, false);
            }
        } break;
        case TextInputCommand::MoveToNextLine: {
            const auto &cursor = d->engine->cursor;
            auto        block  = d->engine->current_block();
            if (d->engine->active_block_index + 1 == d->engine->active_blocks.size()
                && cursor.row + 1 == block->lines.size()) {
                move(block->text_len() - cursor.pos, false);
            } else if (cursor.row + 1 == block->lines.size()) {
                const auto next_block = d->engine->active_blocks[d->engine->active_block_index + 1];
                const int  equiv_col  = cursor.row == 0 ? cursor.col : cursor.col - 2;
                const int  col        = qBound(0, equiv_col, next_block->len_of_line(0));
                move(block->text_len() - cursor.pos + col + 1, false);
            } else {
                const int equiv_col = cursor.row == 0 ? cursor.col + 2 : cursor.col;
                const int col       = qMin(equiv_col, block->len_of_line(cursor.row + 1));
                const int pos       = block->offset_of_line(cursor.row + 1) + col;
                move(pos - cursor.pos, false);
            }
        } break;
        case TextInputCommand::MoveToStartOfLine: {
            const auto block = d->engine->current_block();
            const auto line  = block->current_line();
            move_to(block->text_pos + line.text_offset(), false);
        } break;
        case TextInputCommand::MoveToEndOfLine: {
            const auto block = d->engine->current_block();
            const auto line  = block->current_line();
            const auto pos   = block->text_pos + line.text_offset() + line.text().length();
            move_to(pos, false);
        } break;
        case TextInputCommand::MoveToStartOfBlock: {
            move_to(d->engine->current_block()->text_pos, false);
        } break;
        case TextInputCommand::MoveToEndOfBlock: {
            const auto block = d->engine->current_block();
            move_to(block->text_pos + block->text_len(), false);
        } break;
        case TextInputCommand::MoveToStartOfDocument: {
            move_to(0, false);
            scoll_to_start();
        } break;
        case TextInputCommand::MoveToEndOfDocument: {
            move_to(d->engine->text_ref->length(), false);
            scroll_to_end();
        } break;
        case TextInputCommand::MoveToPrevPage: {
            scroll(get_text_area().height() * 0.5, true);
        } break;
        case TextInputCommand::MoveToNextPage: {
            scroll(-get_text_area().height() * 0.5, true);
        } break;
        case TextInputCommand::MoveToPrevBlock: {
            if (d->engine->active_block_index > 0) {
                --d->engine->active_block_index;
                d->engine->cursor.reset();
                d->cursor_pos = d->engine->current_block()->text_pos;
                request_update();
            }
        } break;
        case TextInputCommand::MoveToNextBlock: {
            if (d->engine->active_block_index + 1 < d->engine->active_blocks.size()) {
                ++d->engine->active_block_index;
                d->engine->cursor.reset();
                d->cursor_pos = d->engine->current_block()->text_pos;
                request_update();
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
            const auto &block  = d->engine->current_block();
            const auto &cursor = d->engine->cursor;
            int         times  = cursor.col;
            if (times == 0) {
                if (cursor.row > 0) {
                    times = block->len_of_line(cursor.row - 1);
                } else {
                    times = 1;
                }
            }
            del(-times);
        } break;
        case TextInputCommand::DeleteToEndOfLine: {
            const auto &block  = d->engine->current_block();
            const auto &cursor = d->engine->cursor;
            int         times  = block->len_of_line(cursor.row) - cursor.col;
            if (times == 0) {
                if (cursor.row + 1 == block->lines.size()) {
                    times = 1;
                } else {
                    times = block->len_of_line(cursor.row + 1);
                }
            }
            del(times);
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
            move(-1, true);
        } break;
        case TextInputCommand::SelectNextChar: {
            move(1, true);
        } break;
        case TextInputCommand::SelectPrevWord: {
        } break;
        case TextInputCommand::SelectNextWord: {
        } break;
        case TextInputCommand::SelectToStartOfLine: {
            move(-cursor.col, true);
        } break;
        case TextInputCommand::SelectToEndOfLine: {
            move(d->engine->current_line().text().length() - cursor.col, true);
        } break;
        case TextInputCommand::SelectToStartOfBlock: {
            move_to(d->engine->current_block()->text_pos, true);
        } break;
        case TextInputCommand::SelectToEndOfBlock: {
            const auto block = d->engine->current_block();
            move_to(block->text_pos + block->text_len(), true);
        } break;
        case TextInputCommand::SelectBlock: {
            const auto block = d->engine->current_block();
            move_to(block->text_pos, false);
            move_to(block->text_pos + block->text_len(), true);
        } break;
        case TextInputCommand::SelectPrevPage: {
        } break;
        case TextInputCommand::SelectNextPage: {
        } break;
        case TextInputCommand::SelectToStartOfDoc: {
            move_to(0, true);
        } break;
        case TextInputCommand::SelectToEndOfDoc: {
            move_to(d->engine->text_ref->length(), true);
        } break;
        case TextInputCommand::SelectAll: {
            move_to(0, false);
            move_to(d->engine->text_ref->length(), true);
        } break;
        case TextInputCommand::InsertBeforeBlock: {
            d->engine->insert_block(d->engine->active_block_index);
            --d->engine->active_block_index;
            d->engine->cursor.reset();
            d->cursor_pos = d->engine->current_block()->text_pos;
            request_update();
        } break;
        case TextInputCommand::InsertAfterBlock: {
            d->engine->insert_block(d->engine->active_block_index + 1);
            auto block = d->engine->current_block();
            move(block->text_len() - d->engine->cursor.pos + 1, false);
        } break;
    }

    jwrite_profiler_record(GeneralTextEdit);

    e->accept();
}

void LimitedViewEditor::mousePressEvent(QMouseEvent *e) {
    QWidget::mousePressEvent(e);

    bool cancel_auto_scroll = false;

    if (e->button() == Qt::MiddleButton) {
        d->auto_scroll_mode = !d->auto_scroll_mode;
    } else if (d->auto_scroll_mode) {
        cancel_auto_scroll  = true;
        d->auto_scroll_mode = false;
    }

    if (d->auto_scroll_mode) {
        set_cursor_shape(Qt::SizeVerCursor);
        d->auto_scroll_mode  = true;
        d->scroll_base_y_pos = e->pos().y();
        d->scroll_ref_y_pos  = d->scroll_base_y_pos;
        d->auto_scroll_timer.start();
        return;
    } else {
        d->auto_scroll_mode = false;
        d->auto_scroll_timer.stop();
        restore_cursor_shape();
    }

    //! NOTE: do not perform the locating action if the current click is to cancel the auto scroll
    //! mode, so that user could have more choices when they scroll to a different view pos
    if (cancel_auto_scroll) { return; }

    if (e->button() != Qt::LeftButton) { return; }

    unset_sel();

    if (const auto e = d->engine; e->is_empty() || e->is_dirty()) { return; }
    if (d->engine->preedit) { return; }

    const auto loc = get_text_loc_at_vpos(e->pos());
    if (loc.block_index == -1) { return; }

    auto &active_block_index = d->engine->active_block_index;
    auto &cursor             = d->engine->cursor;
    if (active_block_index == loc.block_index && cursor.row == loc.row && cursor.col == loc.col) {
        return;
    }

    active_block_index = loc.block_index;
    cursor.row         = loc.row;
    cursor.col         = loc.col;
    cursor.pos         = d->engine->current_line().text_offset() + cursor.col;
    d->cursor_pos      = d->engine->current_block()->text_pos + cursor.pos;

    request_update();
}

void LimitedViewEditor::mouseReleaseEvent(QMouseEvent *e) {
    QWidget::mouseReleaseEvent(e);
}

void LimitedViewEditor::mouseDoubleClickEvent(QMouseEvent *e) {
    QWidget::mouseDoubleClickEvent(e);
    unset_sel();
}

void LimitedViewEditor::mouseMoveEvent(QMouseEvent *e) {
    QWidget::mouseMoveEvent(e);

    if (d->auto_scroll_mode) {
        d->scroll_ref_y_pos = e->pos().y();
        return;
    }

    if (e->buttons() & Qt::LeftButton) {
        do {
            if (!d->engine->is_cursor_available()) { break; }
            if (d->engine->preedit) { break; }
            const auto   bb = get_text_area();
            const QPoint pos{
                qBound(bb.left(), e->pos().x(), bb.right()),
                qBound(bb.top(), e->pos().y(), bb.bottom()),
            };
            const auto loc = get_text_loc_at_vpos(pos);
            if (loc.block_index == -1) { break; }
            auto &active_block_index = d->engine->active_block_index;
            auto &cursor             = d->engine->cursor;
            active_block_index       = loc.block_index;
            cursor.row               = loc.row;
            cursor.col               = loc.col;
            const auto block         = d->engine->current_block();
            const auto line          = block->current_line();
            cursor.pos               = line.text_offset() + cursor.col;
            if (!has_sel()) { d->selected_from = d->cursor_pos; }
            d->cursor_pos  = block->text_pos + cursor.pos;
            d->selected_to = d->cursor_pos;
            request_update();
        } while (0);
    }

    const auto &area = get_text_area();
    if (area.contains(e->pos())) {
        set_cursor_shape(Qt::IBeamCursor);
    } else {
        set_cursor_shape(Qt::ArrowCursor);
    }
}

void LimitedViewEditor::wheelEvent(QWheelEvent *e) {
    const auto engine = d->engine;
    if (engine->is_empty()) { return; }
    const auto ratio = 1.0 / 180 * 3;
    auto       delta = e->angleDelta().y() * engine->line_height * ratio;
    scroll((e->modifiers() & Qt::ControlModifier) ? delta * 8 : delta, true);
}

void LimitedViewEditor::inputMethodEvent(QInputMethodEvent *e) {
    if (!d->engine->is_cursor_available()) { return; }

    const auto &cursor                = d->engine->cursor;
    const auto &saved_cursor          = d->engine->saved_cursor;
    const auto  last_preedit_text_len = cursor.pos - saved_cursor.pos;

    //! FIXME: methods for IME preedit introduce some unexpected bugs, but i can't concretely
    //! describe them so far

    if (const auto preedit_text = e->preeditString(); !preedit_text.isEmpty()) {
        if (!d->engine->preedit) {
            d->engine->begin_preedit(d->preedit_text);
        } else {
            d->preedit_text.remove(saved_cursor.pos, last_preedit_text_len);
        }
        d->preedit_text.insert(saved_cursor.pos, preedit_text);
        d->engine->update_preedit_text(preedit_text.length());
        request_update();
        jwrite_profiler_start(IME2UpdateDelay);
    } else {
        if (d->engine->preedit) { d->engine->commit_preedit(); }
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
