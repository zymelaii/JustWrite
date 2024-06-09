#include <jwrite/ui/Editor.h>
#include <jwrite/TextViewEngine.h>
#include <jwrite/TextInputCommand.h>
#include <jwrite/ProfileUtils.h>
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
#include <spdlog/spdlog.h>

namespace jwrite::ui {

bool Editor::softCenterMode() const {
    return soft_center_mode_;
}

void Editor::setSoftCenterMode(bool value) {
    soft_center_mode_ = value;
    if (soft_center_mode_) {
        const auto min_margin     = 32;
        const auto max_text_width = 1000;
        const auto mean_width     = qMax(0, width() - min_margin * 2);
        const auto text_width     = qMin<int>(mean_width * 0.8, max_text_width);
        const auto margin         = (width() - text_width) / 2;
        ui_margins_               = QMargins(margin, 4, margin, 4);
    } else {
        ui_margins_ = QMargins(4, 4, 4, 4);
    }
    emit textAreaChanged(textArea());
}

QRect Editor::textArea() const {
    return contentsRect() - ui_margins_;
}

void Editor::reset(QString &text, bool swap) {
    context_->quit_preedit();

    auto text_out = this->text();
    context_->edit_text.clear();

    int active_block_index = context_->engine.active_block_index != -1 ? 0 : -1;

    last_text_loc_ = std::nullopt;

    context_->viewport_y_pos = 0;
    context_->engine.clear_all();
    context_->engine.insert_block(0);
    context_->engine.active_block_index = 0;
    context_->engine.cursor.reset();

    context_->edit_cursor_pos = 0;

    context_->cached_render_data_ready = false;
    context_->cursor_moved             = true;
    context_->vertical_move_state      = false;
    context_->unset_sel();

    direct_batch_insert(text);

    context_->engine.active_block_index = active_block_index;
    context_->edit_cursor_pos           = 0;
    context_->engine.cursor.reset();

    if (swap) { text.swap(text_out); }
}

QString Editor::take() {
    context_->quit_preedit();

    const auto text = this->text();

    context_->engine.clear_all();
    context_->edit_text.clear();
    context_->unset_sel();
    context_->cached_render_data_ready = false;
    context_->cursor_moved             = true;
    context_->vertical_move_state      = false;

    history_.clear();

    last_text_loc_ = std::nullopt;

    return std::move(text);
}

void Editor::scrollToCursor() {
    const auto &d = context_->cached_render_state;
    const auto &e = context_->engine;
    Q_ASSERT(e.is_cursor_available());
    Q_ASSERT(context_->cached_render_data_ready);

    const auto   viewport     = textArea();
    const auto  &line         = e.current_line();
    const auto  &cursor       = e.cursor;
    const double line_spacing = e.line_height * e.line_spacing_ratio;

    double y_pos = 0.0;
    if (d.active_block_visible) {
        y_pos = d.active_block_y_start;
    } else if (!d.found_visible_block) {
        for (int index = 0; index < e.active_block_index; ++index) {
            y_pos += e.block_spacing + line_spacing * e.active_blocks[index]->lines.size();
        }
    } else if (e.active_block_index < d.visible_block.first) {
        y_pos = d.cached_block_y_pos[d.visible_block.first];
        for (int index = d.visible_block.first; index > e.active_block_index; --index) {
            y_pos -= e.block_spacing + line_spacing * e.active_blocks[index]->lines.size();
        }
    } else if (e.active_block_index > d.visible_block.last) {
        y_pos = d.cached_block_y_pos[d.visible_block.last];
        for (int index = d.visible_block.last; index < e.active_block_index; ++index) {
            y_pos += e.block_spacing + line_spacing * e.active_blocks[index]->lines.size();
        }
    }

    y_pos += cursor.row * line_spacing;

    const double slack       = e.block_spacing * 0.75;
    const double y_pos_start = y_pos - slack;
    const double y_pos_end   = y_pos + line_spacing + slack;

    if (const double top = context_->viewport_y_pos; y_pos_start < top) {
        scrollTo(y_pos_start, true);
    } else if (const double bottom = context_->viewport_y_pos + context_->viewport_height;
               y_pos_end > bottom) {
        scrollTo(y_pos_end - context_->viewport_height, true);
    }
}

QString Editor::text() const {
    auto      &lock   = context_->lock;
    const bool locked = !lock.on_write() && lock.try_lock_read();

    QStringList blocks{};
    for (auto block : context_->engine.active_blocks) { blocks << block->text().toString(); }

    if (locked) { lock.unlock_read(); }

    return blocks.join("\n");
}

VisualTextEditContext::TextLoc Editor::currentTextLoc() const {
    return context_->current_textloc();
}

void Editor::setCursorToTextLoc(const VisualTextEditContext::TextLoc &loc) {
    context_->set_cursor_to_textloc(loc, 0);
}

QPair<double, double> Editor::scrollBound() const {
    const auto  &e            = context_->engine;
    const double line_spacing = e.line_spacing_ratio * e.line_height;

    //! see drawHighlightBlock(QPainter *p)
    const double line_gap  = line_spacing + e.block_spacing;
    const double slack     = e.block_spacing * 0.75;
    const double min_y_pos = -line_gap;
    double       max_y_pos = -line_gap - slack;
    for (auto block : e.active_blocks) {
        max_y_pos += block->lines.size() * line_spacing + e.block_spacing;
    }

    return {min_y_pos, max_y_pos};
}

void Editor::scroll(double delta, bool smooth) {
    const auto [min_y_pos, max_y_pos] = scrollBound();
    expected_scroll_ = qBound(min_y_pos, context_->viewport_y_pos + delta, max_y_pos);
    if (!smooth) { context_->scroll_to(expected_scroll_); }
    requestUpdate(true);
}

void Editor::scrollTo(double pos_y, bool smooth) {
    const auto [min_y_pos, max_y_pos] = scrollBound();
    expected_scroll_                  = qBound(min_y_pos, pos_y, max_y_pos);
    if (!smooth) { context_->scroll_to(expected_scroll_); }
    requestUpdate(true);
}

void Editor::scrollToStart() {
    const auto [min_y_pos, _] = scrollBound();
    expected_scroll_          = min_y_pos;
    context_->scroll_to(expected_scroll_);
    requestUpdate(true);
}

void Editor::scrollToEnd() {
    const auto [_, max_y_pos] = scrollBound();
    const double line_spacing = context_->engine.line_spacing_ratio * context_->engine.line_height;
    const double y_pos        = max_y_pos + line_spacing - context_->viewport_height / 2;
    scrollTo(y_pos, false);
}

void Editor::direct_remove_sel(QString *deleted_text) {
    Q_ASSERT(context_->has_sel());
    context_->remove_sel_region(deleted_text);
}

void Editor::direct_delete(int times, QString *deleted_text) {
    Q_ASSERT(!context_->has_sel());
    context_->del(times, false, deleted_text);
}

void Editor::execute_delete_action(int times) {
    Q_ASSERT(!context_->engine.preedit);
    Q_ASSERT(context_->engine.is_cursor_available());
    QString deleted_text{};
    if (context_->has_sel()) {
        direct_remove_sel(&deleted_text);
    } else {
        direct_delete(times, &deleted_text);
    }
    const auto loc = currentTextLoc();
    history_.push(TextEditAction::from_action(TextEditAction::Type::Delete, loc, deleted_text));
}

void Editor::direct_insert(const QString &text) {
    Q_ASSERT(!context_->engine.preedit);
    Q_ASSERT(!context_->has_sel());
    context_->insert(text);
}

void Editor::direct_batch_insert(const QString &multiline_text) {
    Q_ASSERT(!context_->engine.preedit);
    Q_ASSERT(!context_->has_sel());
    auto lines = multiline_text.split('\n');
    direct_insert(lines.first());
    for (int i = 1; i < lines.size(); ++i) {
        context_->engine.break_block_at_cursor_pos();
        direct_insert(lines[i]);
    }
}

void Editor::execute_insert_action(const QString &text, bool batch_mode) {
    Q_ASSERT(context_->engine.is_cursor_available());
    Q_ASSERT(!context_->engine.preedit);
    Q_ASSERT(!context_->has_sel());
    const auto loc = currentTextLoc();
    if (batch_mode) {
        direct_batch_insert(text);
    } else {
        direct_insert(text);
    }
    history_.push(TextEditAction::from_action(TextEditAction::Type::Insert, loc, text));
}

bool Editor::insert_action_filter(const QString &text) {
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
        execute_insert_action(text + matched, false);
        context_->move(-1, false);
        return true;
    }

    const auto &e = context_->engine;
    if (auto index = QUOTE_PAIRS.values().indexOf(text); index != -1) {
        const int pos = context_->edit_cursor_pos;
        if (pos == context_->edit_text.length()) { return false; }
        if (context_->edit_text.at(pos) == text) { return true; }
    }

    return false;
}

void Editor::del(int times) {
    execute_delete_action(times);
    emit textChanged(context_->edit_text);
    requestUpdate(true);
}

void Editor::insert(const QString &text, bool batch_mode) {
    Q_ASSERT(context_->engine.is_cursor_available());
    if (context_->has_sel()) {
        Q_ASSERT(!context_->engine.preedit);
        execute_delete_action(0);
    } else if (context_->engine.preedit) {
        context_->commit_preedit();
    }
    if (batch_mode) {
        execute_insert_action(text, true);
        if (!text.isEmpty()) { context_->cursor_moved = true; }
    } else {
        const auto block_text  = context_->engine.current_block()->text();
        const auto insert_pos  = context_->engine.cursor.pos;
        const auto text_before = block_text.left(insert_pos);
        const auto text_after  = block_text.right(block_text.length() - insert_pos);
        const auto text_in     = restrict_rule_->restrict(text, text_before, text_after);

        bool filtered = false;
        if (inserted_filter_enabled_) { filtered = insert_action_filter(text_in); }

        if (!filtered) { execute_insert_action(text_in, false); }
    }
    emit textChanged(context_->edit_text);
    requestUpdate(true);
}

void Editor::select(int start_pos, int end_pos) {
    auto &sel = context_->sel;
    sel.clear();
    sel.from                           = qBound(0, start_pos, context_->edit_text.length());
    sel.to                             = qBound(0, end_pos, context_->edit_text.length());
    context_->cached_render_data_ready = false;
    requestUpdate(true);
}

void Editor::move(int offset, bool extend_sel) {
    context_->move(offset, extend_sel);
    requestUpdate(true);
}

void Editor::move_to(int pos, bool extend_sel) {
    context_->move_to(pos, extend_sel);
    requestUpdate(true);
}

void Editor::copy() {
    auto clipboard = QGuiApplication::clipboard();
    if (context_->has_sel()) {
        const auto &e        = context_->engine;
        const int   pos_from = qMin(context_->sel.from, context_->sel.to);
        const int   pos_to   = qMax(context_->sel.from, context_->sel.to);
        const auto  loc_from = context_->get_textloc_at_pos(pos_from, -1);
        const auto  loc_to   = context_->get_textloc_at_pos(pos_to, 1);
        QStringList copied_text{};
        for (int index = loc_from.block_index; index <= loc_to.block_index; ++index) {
            const auto block = e.active_blocks[index];
            const int  from  = index == loc_from.block_index ? loc_from.pos : 0;
            const int  to    = index == loc_to.block_index ? loc_to.pos : block->text_len();
            copied_text << block->text().mid(from, to - from).toString();
        }
        clipboard->setText(copied_text.join('\n'));
    } else if (const auto &e = context_->engine; e.is_cursor_available()) {
        //! copy the current block if has no sel
        const auto copied_text = e.current_block()->text();
        clipboard->setText(copied_text.toString());
    }
}

void Editor::cut() {
    copy();
    if (context_->has_sel()) {
        del(0);
    } else {
        //! cut the current block if has no sel, also remove the block
        const auto &e = context_->engine;
        context_->move(-e.cursor.pos, false);
        del(e.current_block()->text_len() + 1);
    }
}

void Editor::paste() {
    auto clipboard = QGuiApplication::clipboard();
    auto mime      = clipboard->mimeData();
    if (!mime->hasText()) { return; }
    insert(clipboard->text(), true);
}

void Editor::undo() {
    if (auto opt = history_.get_undo_action()) {
        auto action = opt.value();
        context_->unset_sel();
        setCursorToTextLoc(action.loc);
        switch (action.type) {
            case TextEditAction::Insert: {
                direct_batch_insert(action.text);
            } break;
            case TextEditAction::Delete: {
                direct_delete(action.text.length(), nullptr);
            } break;
        }
        emit textChanged(context_->edit_text);
        requestUpdate(true);
    }
}

void Editor::redo() {
    if (auto opt = history_.get_redo_action()) {
        auto action = opt.value();
        context_->unset_sel();
        setCursorToTextLoc(action.loc);
        switch (action.type) {
            case TextEditAction::Insert: {
                direct_batch_insert(action.text);
            } break;
            case TextEditAction::Delete: {
                direct_delete(action.text.length(), nullptr);
            } break;
        }
        emit textChanged(context_->edit_text);
        requestUpdate(true);
    }
}

void Editor::breakIntoNewLine(bool should_update) {
    if (context_->engine.current_block()->text_len() == 0) { return; }
    context_->remove_sel_region(nullptr);
    context_->engine.break_block_at_cursor_pos();
    context_->cursor_moved = true;
    if (should_update) { requestUpdate(true); }
}

void Editor::verticalMove(bool up) {
    context_->vertical_move(up);
    requestUpdate(true);
}

Tokenizer *Editor::tokenizer() const {
    if (!tokenizer_) { const_cast<Editor *>(this)->tokenizer_ = fut_tokenizer_.result(); }
    return tokenizer_;
}

void Editor::setTimerEnabled(bool enabled) {
    if (enabled) {
        stable_timer_.start();
        blink_timer_.start();
    } else {
        stable_timer_.stop();
        blink_timer_.stop();
    }
    timer_enabled_ = enabled;
}

void Editor::renderBlinkCursor() {
    blink_cursor_should_paint_ = !blink_cursor_should_paint_;
    requestUpdate(false);
}

void Editor::render() {
    if (auto_scroll_mode_) {
        scroll((scroll_ref_y_pos_ - scroll_base_y_pos_) / 10, false);
        update_requested_ = true;
    }

    if (drag_sel_flag_ && oob_drag_sel_flag_) { updateTextLocToVisualPos(oob_drag_sel_vpos_); }

    if (update_requested_) {
        update();
        update_requested_ = false;
    }
}

QSize Editor::sizeHint() const {
    return minimumSizeHint();
}

QSize Editor::minimumSizeHint() const {
    const auto margins      = contentsMargins() + ui_margins_;
    const auto line_spacing = context_->engine.line_height * context_->engine.line_spacing_ratio;
    const auto hori_margin  = margins.left() + margins.right();
    const auto vert_margin  = margins.top() + margins.bottom();
    const auto min_width =
        min_text_line_chars_ * context_->engine.standard_char_width + hori_margin;
    const auto min_height = line_spacing * 3 + context_->engine.block_spacing * 2 + vert_margin;
    return QSize(min_width, min_height);
}

Editor::Editor(QWidget *parent)
    : QWidget(parent) {
    init();
}

Editor::~Editor() {
    delete context_;
    delete restrict_rule_;
    delete tokenizer_;
    delete input_manager_;
}

void Editor::init() {
    setFocusPolicy(Qt::ClickFocus);
    setAttribute(Qt::WA_InputMethodEnabled);
    setAutoFillBackground(true);
    setAcceptDrops(true);
    setMouseTracking(true);
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    setContentsMargins({});

    min_text_line_chars_       = 12;
    soft_center_mode_          = false;
    expected_scroll_           = 0.0;
    blink_cursor_should_paint_ = true;
    inserted_filter_enabled_   = true;
    drag_sel_flag_             = false;
    oob_drag_sel_flag_         = false;
    auto_scroll_mode_          = false;
    ui_cursor_shape_[0]        = Qt::ArrowCursor;
    ui_cursor_shape_[1]        = Qt::ArrowCursor;

    setSoftCenterMode(true);

    const auto text_area = textArea();
    context_             = new VisualTextEditContext(fontMetrics(), text_area.width());
    context_->resize_viewport(context_->viewport_width, text_area.height());
    context_->viewport_y_pos = 0;

    restrict_rule_ = new TextRestrictRule;
    tokenizer_     = nullptr;
    fut_tokenizer_ = std::move(Tokenizer::build());

    timer_enabled_ = true;

    update_requested_ = false;
    stable_timer_.setInterval(16);
    stable_timer_.setSingleShot(false);

    blink_timer_.setInterval(500);
    blink_timer_.setSingleShot(false);

    input_manager_ = new GeneralTextInputCommandManager(context_->engine);
    input_manager_->load_default();

    scrollToStart();

    connect(this, &Editor::textAreaChanged, this, [this](QRect area) {
        context_->resize_viewport(area.width(), area.height());
        requestUpdate(false);
    });
    connect(&blink_timer_, &QTimer::timeout, this, &Editor::renderBlinkCursor);
    connect(&stable_timer_, &QTimer::timeout, this, &Editor::render);

    stable_timer_.start();
}

void Editor::requestUpdate(bool sync) {
    if (sync) {
        update_requested_          = true;
        blink_cursor_should_paint_ = true;
        if (timer_enabled_) {
            blink_timer_.stop();
            blink_timer_.start();
        }
    } else {
        update_requested_ = false;
        update();
    }
}

void Editor::setCursorShape(Qt::CursorShape shape) {
    ui_cursor_shape_[1] = ui_cursor_shape_[0];
    ui_cursor_shape_[0] = shape;
    setCursor(shape);
}

void Editor::restoreCursorShape() {
    setCursorShape(ui_cursor_shape_[1]);
}

void Editor::drawTextArea(QPainter *p) {
    const auto &d = context_->cached_render_state;
    if (!d.found_visible_block) { return; }

    jwrite_profiler_start(TextBodyRenderCost);

    const auto pal   = palette();
    const auto flags = Qt::AlignBaseline | Qt::TextDontClip;

    p->save();
    p->setPen(pal.color(QPalette::Text));

    const auto  &e            = context_->engine;
    const double indent       = e.standard_char_width * 2;
    const double line_spacing = e.line_height * e.line_spacing_ratio;
    const auto   viewport     = textArea();

    QRectF bb(viewport.left(), viewport.top(), viewport.width(), e.line_height);
    bb.translate(0, d.first_visible_block_y_pos - context_->viewport_y_pos);

    const auto y_pos = d.first_visible_block_y_pos - context_->viewport_y_pos;
    auto       pos   = viewport.topLeft() + QPointF(0, y_pos);

    for (int index = d.visible_block.first; index <= d.visible_block.last; ++index) {
        const auto block = e.active_blocks[index];

        for (const auto &line : block->lines) {
            const double leading_space = line.is_first_line() ? indent : 0;
            const double spacing       = line.char_spacing();
            bb.setLeft(viewport.left() + leading_space);
            for (const auto c : line.text()) {
                p->drawText(bb, flags, c);
                const double advance = e.fm.horizontalAdvance(c);
                bb.adjust(advance + spacing, 0, 0, 0);
            }
            bb.translate(0, line_spacing);
        }
        bb.translate(0, e.block_spacing);
    }

    p->restore();

    jwrite_profiler_record(TextBodyRenderCost);
}

bool Editor::updateTextLocToVisualPos(const QPoint &vpos) {
    const auto &e   = context_->engine;
    const auto  loc = context_->get_textloc_at_rel_vpos(vpos, true);
    Q_ASSERT(loc.block_index != -1);
    const auto block = e.active_blocks[loc.block_index];
    move_to(block->text_pos + loc.pos, true);
    const double line_spacing  = e.line_height * e.line_spacing_ratio;
    bool         out_of_bounds = true;
    if (vpos.y() < 0) {
        scroll(-line_spacing * 0.5, true);
    } else if (vpos.y() > context_->viewport_height) {
        scroll(line_spacing * 0.5, true);
    } else {
        out_of_bounds = false;
    }
    return out_of_bounds;
}

void Editor::stopDragAndSelect() {
    drag_sel_flag_     = false;
    oob_drag_sel_flag_ = false;
}

void Editor::drawSelection(QPainter *p) {
    const auto &e = context_->engine;
    const auto &d = context_->cached_render_state;
    if (!context_->has_sel()) { return; }
    if (d.visible_sel.first == -1) { return; }

    jwrite_profiler_start(SelectionAreaRenderCost);

    const auto pal = palette();

    const auto   viewport     = textArea();
    const double line_spacing = e.line_height * e.line_spacing_ratio;

    double y_pos = d.cached_block_y_pos[d.visible_sel.first];

    for (int index = d.visible_sel.first; index <= d.visible_sel.last; ++index) {
        const auto   block         = e.active_blocks[index];
        const bool   is_first      = index == d.sel_loc_from.block_index;
        const bool   is_last       = index == d.sel_loc_to.block_index;
        const int    line_nr_begin = is_first ? d.sel_loc_from.row : 0;
        const int    line_nr_end   = is_last ? d.sel_loc_to.row : block->lines.size() - 1;
        const double stride        = line_nr_begin * line_spacing;

        double y_pos = d.cached_block_y_pos[index] + stride - context_->viewport_y_pos;
        //! add the minus part to make the selection area align center
        y_pos -= e.fm.descent() * 0.5;

        for (int line_nr = line_nr_begin; line_nr <= line_nr_end; ++line_nr) {
            const auto line   = block->lines[line_nr];
            const auto offset = line.is_first_line() ? e.standard_char_width * 2 : 0;

            double x_pos = offset;
            if (is_first && line_nr == d.sel_loc_from.row) {
                x_pos += line.char_spacing() * d.sel_loc_from.col;
                for (const auto &c : line.text().left(d.sel_loc_from.col)) {
                    x_pos += e.fm.horizontalAdvance(c);
                }
            }

            double sel_width = line.cached_text_width + (line.text_len() - 1) * line.char_spacing()
                             - (x_pos - offset);
            if (is_last && line_nr == d.sel_loc_to.row) {
                sel_width -= line.char_spacing() * (line.text_len() - d.sel_loc_to.col);
                for (const auto &c : line.text().mid(d.sel_loc_to.col)) {
                    sel_width -= e.fm.horizontalAdvance(c);
                }
            }

            QRectF bb(x_pos, y_pos, sel_width, e.line_height);
            bb.translate(viewport.topLeft());
            p->fillRect(bb, pal.highlight());

            y_pos += line_spacing;
        }
    }

    jwrite_profiler_record(SelectionAreaRenderCost);
}

void Editor::drawHighlightBlock(QPainter *p) {
    const auto &d = context_->cached_render_state;
    const auto &e = context_->engine;
    if (!e.is_cursor_available() || context_->has_sel()) { return; }
    if (!d.active_block_visible) { return; }

    const auto pal = palette();

    p->save();
    p->setPen(Qt::transparent);
    p->setBrush(pal.highlightedText());

    const auto viewport = textArea();

    const double line_spacing = e.line_height * e.line_spacing_ratio;
    const double line_slack   = qMax(0.0, line_spacing - e.line_height);
    const double start_y_pos  = d.active_block_y_start - context_->viewport_y_pos - line_slack;
    const double end_y_pos    = d.active_block_y_end - context_->viewport_y_pos - e.block_spacing;

    const double height  = end_y_pos - start_y_pos;
    const double w_slack = 8.0;
    const double h_slack = context_->engine.block_spacing * 0.75;
    const int    radius  = 4;

    QRectF bb(0, start_y_pos, context_->viewport_width, height);
    bb.translate(viewport.topLeft());
    bb.adjust(-w_slack, -h_slack, w_slack, h_slack);

    p->drawRoundedRect(bb, radius, radius);

    p->restore();
}

void Editor::drawCursor(QPainter *p) {
    const auto &d = context_->cached_render_state;
    const auto &e = context_->engine;
    if (!(e.is_cursor_available() && blink_cursor_should_paint_)) { return; }
    if (!d.active_block_visible) { return; }

    jwrite_profiler_start(CursorRenderCost);

    const auto   viewport     = textArea();
    const auto  &line         = e.current_line();
    const auto  &cursor       = e.cursor;
    const double line_spacing = e.line_height * e.line_spacing_ratio;
    const double y_pos        = d.active_block_y_start + cursor.row * line_spacing;

    //! NOTE: you may question about why it doesn't call `fm.horizontalAdvance(text)`
    //! directly, and the reason is that the text_width calcualated by that has a few
    //! difference with the render result of the text, and the cursor will seems not in the
    //! correct place, and this problem was extremely serious in pure latin texts
    const double leading_space = line.is_first_line() ? e.standard_char_width * 2 : 0;
    double       cursor_x_pos  = leading_space + line.char_spacing() * cursor.col;
    for (const auto &c : line.text().left(cursor.col)) {
        cursor_x_pos += e.fm.horizontalAdvance(c);
    }
    const double cursor_y_pos = y_pos - context_->viewport_y_pos;
    const auto   cursor_pos   = QPoint(cursor_x_pos, cursor_y_pos) + viewport.topLeft();

    p->save();

    //! NOTE: set pen width less than 1 to ensure a single pixel cursor
    p->setPen(QPen(palette().text(), 0.8));

    p->drawLine(cursor_pos, cursor_pos + QPoint(0, e.fm.height()));

    p->restore();

    jwrite_profiler_record(CursorRenderCost);
}

void Editor::resizeEvent(QResizeEvent *e) {
    QWidget::resizeEvent(e);
    if (soft_center_mode_) { setSoftCenterMode(true); }
    emit textAreaChanged(textArea());
}

void Editor::paintEvent(QPaintEvent *e) {
    //! smooth scroll
    if (qAbs(context_->viewport_y_pos - expected_scroll_) > 1e-3) {
        const double new_scroll_pos = context_->viewport_y_pos * 0.49 + expected_scroll_ * 0.51;
        const double scroll_delta   = new_scroll_pos - context_->viewport_y_pos;
        if (qAbs(scroll_delta) < 10) {
            context_->scroll_to(expected_scroll_);
        } else {
            context_->scroll_to(new_scroll_pos);
            update_requested_ = true;
        }
    }

    jwrite_profiler_start(PrepareRenderData);
    context_->prepare_render_data();
    jwrite_profiler_record(PrepareRenderData);

    if (!context_->lock.try_lock_read()) { return; }
    if (!context_->cached_render_data_ready) { return; }

    jwrite_profiler_start(FrameRenderCost);

    QPainter p(this);
    auto     pal = palette();

    //! draw selection
    drawSelection(&p);

    //! draw highlight text block
    drawHighlightBlock(&p);

    //! draw text area
    drawTextArea(&p);

    //! draw cursor
    drawCursor(&p);
    if (context_->engine.is_cursor_available() && context_->cursor_moved) {
        scrollToCursor();
        context_->cursor_moved = false;
    }

    jwrite_profiler_record(FrameRenderCost);

    context_->lock.unlock_read();
}

void Editor::focusInEvent(QFocusEvent *e) {
    QWidget::focusInEvent(e);

    if (auto &engine = context_->engine; engine.is_empty()) {
        engine.insert_block(0);
        //! TODO: move it into a safe method
        engine.active_block_index = 0;
        emit activated();
    } else if (last_text_loc_ && last_text_loc_->block_index != -1) {
        if (e->reason() != Qt::FocusReason::MouseFocusReason) {
            setCursorToTextLoc(*last_text_loc_);
        }
        last_text_loc_ = std::nullopt;
    }

    requestUpdate(true);
}

void Editor::focusOutEvent(QFocusEvent *e) {
    QWidget::focusOutEvent(e);
    context_->unset_sel();
    blink_timer_.stop();

    const auto text_loc = currentTextLoc();
    last_text_loc_      = text_loc;

    context_->engine.active_block_index = -1;

    emit focusLost(text_loc);
}

void Editor::keyPressEvent(QKeyEvent *e) {
    if (!context_->engine.is_cursor_available()) { return; }

    //! ATTENTION: normally this branch is unreachable, but when IME events are too frequent and
    //! the system latency is too high, an IME preedit interrupt may occur and the key is
    //! forwarded to the keyPress event. in this case, we should reject the event or submit the
    //! raw preedit text. the first solution is taken here.
    //! FIXME: the solution is not fully tested to be safe and correct
    if (context_->engine.preedit) { return; }

    const auto action = input_manager_->match(e);
    qDebug() << "COMMAND" << magic_enum::enum_name(action).data();

    auto        &engine       = context_->engine;
    auto        &cursor       = engine.cursor;
    const double line_spacing = engine.line_height * engine.line_spacing_ratio;

    jwrite_profiler_start(GeneralTextEdit);

    switch (action) {
        case TextInputCommand::Reject: {
        } break;
        case TextInputCommand::InsertPrintable: {
            insert(input_manager_->translate_printable_char(e), false);
        } break;
        case TextInputCommand::InsertTab: {
        } break;
        case TextInputCommand::InsertNewLine: {
            insert("\n", true);
        } break;
        case TextInputCommand::Cancel: {
            context_->unset_sel();
            requestUpdate(false);
        } break;
        case TextInputCommand::Undo: {
            undo();
        } break;
        case TextInputCommand::Redo: {
            redo();
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
            scroll(-line_spacing, true);
        } break;
        case TextInputCommand::ScrollDown: {
            scroll(line_spacing, true);
        } break;
        case TextInputCommand::MoveToPrevChar: {
            move(-1, false);
        } break;
        case TextInputCommand::MoveToNextChar: {
            move(1, false);
        } break;
        case TextInputCommand::MoveToPrevWord: {
            const auto block = engine.current_block();
            const int  len   = cursor.pos;
            if (len == 0) {
                move(-1, false);
            } else {
                const auto word   = tokenizer()->get_last_word(block->text().left(len).toString());
                const int  offset = word.length();
                Q_ASSERT(offset <= cursor.pos);
                move_to(block->text_pos + cursor.pos - offset, false);
            }
        } break;
        case TextInputCommand::MoveToNextWord: {
            const auto block = engine.current_block();
            const int  len   = block->text_len() - cursor.pos;
            if (len == 0) {
                move(1, false);
            } else {
                const auto word = tokenizer()->get_first_word(block->text().right(len).toString());
                const int  offset = word.length();
                Q_ASSERT(offset <= len);
                move_to(block->text_pos + cursor.pos + offset, false);
            }
        } break;
        case TextInputCommand::MoveToPrevLine: {
            verticalMove(true);
        } break;
        case TextInputCommand::MoveToNextLine: {
            verticalMove(false);
        } break;
        case TextInputCommand::MoveToStartOfLine: {
            const auto block = engine.current_block();
            const auto line  = block->current_line();
            move_to(block->text_pos + line.text_offset(), false);
        } break;
        case TextInputCommand::MoveToEndOfLine: {
            const auto block = engine.current_block();
            const auto line  = block->current_line();
            const auto pos   = block->text_pos + line.text_offset() + line.text().length();
            move_to(pos, false);
        } break;
        case TextInputCommand::MoveToStartOfBlock: {
            move_to(engine.current_block()->text_pos, false);
        } break;
        case TextInputCommand::MoveToEndOfBlock: {
            const auto block = engine.current_block();
            move_to(block->text_pos + block->text_len(), false);
        } break;
        case TextInputCommand::MoveToStartOfDocument: {
            move_to(0, false);
            scrollToStart();
        } break;
        case TextInputCommand::MoveToEndOfDocument: {
            move_to(engine.text_ref->length(), false);
            scrollToEnd();
        } break;
        case TextInputCommand::MoveToPrevPage: {
            scroll(-textArea().height() * 0.5, true);
        } break;
        case TextInputCommand::MoveToNextPage: {
            scroll(textArea().height() * 0.5, true);
        } break;
        case TextInputCommand::MoveToPrevBlock: {
            if (engine.active_block_index > 0) {
                const auto block      = engine.current_block();
                const auto prev_block = engine.active_blocks[engine.active_block_index - 1];
                move(prev_block->text_pos - block->text_pos - cursor.pos - 1, false);
            }
        } break;
        case TextInputCommand::MoveToNextBlock: {
            if (engine.active_block_index + 1 < engine.active_blocks.size()) {
                const auto block      = engine.current_block();
                const auto next_block = engine.active_blocks[engine.active_block_index + 1];
                move(next_block->text_pos - block->text_pos - cursor.pos + 1, false);
            }
        } break;
        case TextInputCommand::DeletePrevChar: {
            del(-1);
        } break;
        case TextInputCommand::DeleteNextChar: {
            del(1);
        } break;
        case TextInputCommand::DeletePrevWord: {
            const auto block = engine.current_block();
            const int  len   = cursor.pos;
            if (context_->has_sel() || len == 0) {
                del(-1);
            } else {
                const auto word   = tokenizer()->get_last_word(block->text().left(len).toString());
                const int  offset = word.length();
                Q_ASSERT(offset <= cursor.pos);
                del(-offset);
            }
        } break;
        case TextInputCommand::DeleteNextWord: {
            const auto block = engine.current_block();
            const int  len   = block->text_len() - cursor.pos;
            if (context_->has_sel() || len == 0) {
                del(1);
            } else {
                const auto word = tokenizer()->get_first_word(block->text().right(len).toString());
                const int  offset = word.length();
                Q_ASSERT(offset <= len);
                del(offset);
            }
        } break;
        case TextInputCommand::DeleteToStartOfLine: {
            const auto &block = engine.current_block();
            int         times = cursor.col;
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
            const auto &block = engine.current_block();
            int         times = block->len_of_line(cursor.row) - cursor.col;
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
            const auto block = engine.current_block();
            const int  len   = cursor.pos;
            if (len == 0) {
                move(-1, true);
            } else {
                const auto word   = tokenizer()->get_last_word(block->text().left(len).toString());
                const int  offset = word.length();
                Q_ASSERT(offset <= cursor.pos);
                move_to(block->text_pos + cursor.pos - offset, true);
            }
        } break;
        case TextInputCommand::SelectNextWord: {
            const auto block = engine.current_block();
            const int  len   = block->text_len() - cursor.pos;
            if (len == 0) {
                move(1, true);
            } else {
                const auto word = tokenizer()->get_first_word(block->text().right(len).toString());
                const int  offset = word.length();
                Q_ASSERT(offset <= len);
                move_to(block->text_pos + cursor.pos + offset, true);
            }
        } break;
        case TextInputCommand::SelectToPrevLine: {
            auto     &c        = *context_;
            const int sel_from = c.has_sel() ? c.sel.from : c.edit_cursor_pos;
            if (c.has_sel()) { c.unset_sel(); }
            verticalMove(true);
            c.sel.from = sel_from;
            c.sel.to   = c.edit_cursor_pos;
        } break;
        case TextInputCommand::SelectToNextLine: {
            auto     &c        = *context_;
            const int sel_from = c.has_sel() ? c.sel.from : c.edit_cursor_pos;
            if (c.has_sel()) { c.unset_sel(); }
            verticalMove(false);
            c.sel.from = sel_from;
            c.sel.to   = c.edit_cursor_pos;
        } break;
        case TextInputCommand::SelectToStartOfLine: {
            move(-cursor.col, true);
        } break;
        case TextInputCommand::SelectToEndOfLine: {
            move(engine.current_line().text().length() - cursor.col, true);
        } break;
        case TextInputCommand::SelectToStartOfBlock: {
            move_to(engine.current_block()->text_pos, true);
        } break;
        case TextInputCommand::SelectToEndOfBlock: {
            const auto block = engine.current_block();
            move_to(block->text_pos + block->text_len(), true);
        } break;
        case TextInputCommand::SelectBlock: {
            const auto block = engine.current_block();
            select(block->text_pos, block->text_pos + block->text_len());
        } break;
        case TextInputCommand::SelectPrevPage: {
            jwrite_profiler_start(SelectPage);
            const auto   origin = context_->get_vpos_at_cursor();
            const QPoint dest(
                origin.x(), origin.y() - context_->viewport_y_pos - context_->viewport_height);
            const auto dest_loc = context_->get_textloc_at_rel_vpos(dest, false);
            Q_ASSERT(dest_loc.block_index != -1);
            const int pos = engine.active_blocks[dest_loc.block_index]->text_pos + dest_loc.pos;
            move_to(pos, true);
            jwrite_profiler_record(SelectPage);
        } break;
        case TextInputCommand::SelectNextPage: {
            jwrite_profiler_start(SelectPage);
            const auto   origin = context_->get_vpos_at_cursor();
            const QPoint dest(
                origin.x(), origin.y() - context_->viewport_y_pos + context_->viewport_height);
            const auto dest_loc = context_->get_textloc_at_rel_vpos(dest, false);
            Q_ASSERT(dest_loc.block_index != -1);
            const int pos = engine.active_blocks[dest_loc.block_index]->text_pos + dest_loc.pos;
            move_to(pos, true);
            jwrite_profiler_record(SelectPage);
        } break;
        case TextInputCommand::SelectToStartOfDoc: {
            move_to(0, true);
        } break;
        case TextInputCommand::SelectToEndOfDoc: {
            move_to(engine.text_ref->length(), true);
        } break;
        case TextInputCommand::SelectAll: {
            select(0, context_->engine.text_ref->length());
        } break;
        case TextInputCommand::InsertBeforeBlock: {
            if (engine.current_block()->text_len() == 0) { break; }
            const auto block = engine.current_block();
            move_to(block->text_pos, false);
            insert("\n", true);
            move(-1, false);
        } break;
        case TextInputCommand::InsertAfterBlock: {
            if (engine.current_block()->text_len() == 0) { break; }
            const auto block = engine.current_block();
            move_to(block->text_pos + block->text_len(), false);
            insert("\n", true);
        } break;
    }

    jwrite_profiler_record(GeneralTextEdit);
}

void Editor::mousePressEvent(QMouseEvent *e) {
    QWidget::mousePressEvent(e);

    bool cancel_auto_scroll = false;

    if (e->button() == Qt::MiddleButton) {
        auto_scroll_mode_ = !auto_scroll_mode_;
    } else if (auto_scroll_mode_) {
        cancel_auto_scroll = true;
        auto_scroll_mode_  = false;
    }

    if (auto_scroll_mode_) {
        setCursorShape(Qt::SizeVerCursor);
        auto_scroll_mode_  = true;
        scroll_base_y_pos_ = e->pos().y();
        scroll_ref_y_pos_  = scroll_base_y_pos_;
        return;
    } else {
        auto_scroll_mode_ = false;
        restoreCursorShape();
    }

    //! NOTE: do not perform the locating action if the current click is to cancel the auto scroll
    //! mode, so that user could have more choices when they scroll to a different view pos
    if (cancel_auto_scroll) { return; }

    if (e->button() != Qt::LeftButton) {
        stopDragAndSelect();
        return;
    }

    context_->unset_sel();

    auto &engine = context_->engine;

    if (engine.is_empty() || engine.is_dirty()) { return; }
    if (engine.preedit) { return; }

    const auto loc = context_->get_textloc_at_rel_vpos(e->pos() - textArea().topLeft(), true);
    Q_ASSERT(loc.block_index != -1);

    const bool success = context_->set_cursor_to_textloc(loc, 0);
    Q_ASSERT(success);

    requestUpdate(true);

    drag_sel_flag_ = true;
}

void Editor::mouseReleaseEvent(QMouseEvent *e) {
    QWidget::mouseReleaseEvent(e);

    stopDragAndSelect();
}

void Editor::mouseDoubleClickEvent(QMouseEvent *e) {
    QWidget::mouseDoubleClickEvent(e);

    if (context_->engine.preedit) { return; }

    context_->unset_sel();

    if (e->button() == Qt::LeftButton) {
        const auto loc = context_->get_textloc_at_rel_vpos(e->pos() - textArea().topLeft(), true);
        if (loc.block_index != -1) {
            const auto block = context_->engine.active_blocks[loc.block_index];
            select(block->text_pos, block->text_pos + block->text_len());
        }
    }
}

void Editor::mouseMoveEvent(QMouseEvent *e) {
    QWidget::mouseMoveEvent(e);

    if (auto_scroll_mode_) {
        scroll_ref_y_pos_ = e->pos().y();
        return;
    }

    if ((e->buttons() & Qt::LeftButton) && drag_sel_flag_) {
        do {
            auto &engine = context_->engine;
            if (!engine.is_cursor_available()) { break; }
            if (engine.preedit) { break; }
            const auto bb            = textArea();
            const auto vpos          = e->globalPosition().toPoint() - mapToGlobal(bb.topLeft());
            const bool out_of_bounds = updateTextLocToVisualPos(vpos);
            oob_drag_sel_flag_       = out_of_bounds;
            if (oob_drag_sel_flag_) { oob_drag_sel_vpos_ = vpos; }
        } while (0);
    }

    const auto &area = textArea();
    if (area.contains(e->pos())) {
        setCursorShape(Qt::IBeamCursor);
    } else {
        setCursorShape(Qt::ArrowCursor);
    }
}

void Editor::wheelEvent(QWheelEvent *e) {
    const auto &engine = context_->engine;
    if (engine.is_empty()) { return; }
    const double ratio        = 1.0 / 180 * 3;
    const double line_spacing = engine.line_height * engine.line_spacing_ratio;
    const double delta        = -e->angleDelta().y() * line_spacing * ratio;
    scroll((e->modifiers() & Qt::ControlModifier) ? delta * 8 : delta, true);
}

void Editor::inputMethodEvent(QInputMethodEvent *e) {
    auto &engine = context_->engine;
    if (!engine.is_cursor_available()) { return; }
    jwrite_profiler_start(InputMethodEditorResponse);
    if (const auto preedit_text = e->preeditString(); !preedit_text.isEmpty()) {
        if (!engine.preedit) { context_->begin_preedit(); }
        context_->update_preedit(e->preeditString());
    } else {
        insert(e->commitString(), false);
    }
    requestUpdate(true);
    jwrite_profiler_record(InputMethodEditorResponse);
}

void Editor::dragEnterEvent(QDragEnterEvent *e) {
    if (e->mimeData()->hasUrls()) {
        e->acceptProposedAction();
    } else {
        e->ignore();
    }
}

void Editor::dropEvent(QDropEvent *e) {
    QWidget::dropEvent(e);
    const auto urls = e->mimeData()->urls();
    //! TODO: filter plain text files and handle open action
}

} // namespace jwrite::ui
