#include <jwrite/ui/PrimaryEditor.h>
#include <jwrite/AppConfig.h>
#include <QApplication>
#include <QClipboard>
#include <QPainter>

namespace jwrite::ui {

QString PrimaryEditor::text() const {
    auto      &lock   = context()->lock;
    const bool locked = !lock.on_write() && lock.try_lock_read();

    QStringList blocks{};
    for (auto block : engine()->active_blocks) { blocks << block->text().toString(); }

    if (locked) { lock.unlock_read(); }

    return blocks.join("\n");
}

bool PrimaryEditor::enabled() const {
    return enabled_;
}

void PrimaryEditor::set_enabled(bool on) {
    enabled_ = on;
    request_update(true);
    reset_input_method_state();
}

bool PrimaryEditor::readonly() const {
    return readonly_;
}

void PrimaryEditor::set_readonly(bool on) {
    const bool was_on = readonly_;
    readonly_         = on;
    if (was_on != on && on) {
        reset_mouse_action_state();
        request_update(false);
    }
    reset_input_method_state();
}

PrimaryEditor::CaretLoc PrimaryEditor::caret_loc() const {
    return context()->current_textloc();
}

void PrimaryEditor::set_caret_loc(const CaretLoc &loc) {
    context()->set_cursor_to_textloc(loc, 0);
    request_update(false);
}

void PrimaryEditor::set_caret_loc_to_pos(const QPoint &pos, bool extend_sel) {
    auto       ctx = context();
    const auto e   = engine();

    Q_ASSERT(!e->is_empty());
    Q_ASSERT(!e->is_dirty());
    Q_ASSERT(!e->preedit);

    const auto loc = ctx->get_textloc_at_rel_vpos(pos, true);
    Q_ASSERT(loc.block_index != -1);

    if (!extend_sel) {
        if (ctx->has_sel()) { ctx->unset_sel(); }
        const bool succeed = ctx->set_cursor_to_textloc(loc, 0);
        Q_ASSERT(succeed);
    } else {
        const int text_pos = e->text_pos_at_text_loc(loc);
        move_to(text_pos, true);
    }
}

PrimaryEditor::CaretBlinkStyle PrimaryEditor::caret_style() const {
    return caret_blink_.style;
}

void PrimaryEditor::set_caret_style(PrimaryEditor::CaretBlinkStyle style) {
    caret_blink_.style = style;
}

bool PrimaryEditor::insert_filter_enabled() const {
    return insert_filter_enabled_;
}

void PrimaryEditor::set_insert_filter_enabled(bool enabled) {
    insert_filter_enabled_ = enabled;
}

QRect PrimaryEditor::text_area() const {
    return contentsRect() - text_view_margins_;
}

double PrimaryEditor::line_spacing() const {
    const auto e = engine();
    return e->line_height * e->line_spacing_ratio;
}

double PrimaryEditor::line_spacing_ratio() const {
    return engine()->line_spacing_ratio;
}

void PrimaryEditor::set_line_spacing_ratio(double ratio) {
    engine()->line_spacing_ratio        = ratio;
    context()->cached_render_data_ready = false;
    render();
}

double PrimaryEditor::block_spacing() const {
    return engine()->block_spacing;
}

void PrimaryEditor::set_block_spacing(double spacing) {
    engine()->block_spacing             = spacing;
    context()->cached_render_data_ready = false;
    render();
}

int PrimaryEditor::font_size() const {
    return text_font_.pointSize();
}

void PrimaryEditor::set_font_size(int size) {
    text_font_.setPointSize(size);
    engine()->reset_font_metrics(QFontMetrics(text_font_));
    context()->cached_render_data_ready = false;
    render();
}

QFont PrimaryEditor::text_font() const {
    return text_font_;
}

void PrimaryEditor::set_text_font(QFont font) {
    text_font_ = font;
    engine()->reset_font_metrics(QFontMetrics(text_font_));
    context()->cached_render_data_ready = false;
    render();
}

PrimaryEditor::ViewBounds PrimaryEditor::recalc_view_bounds() const {
    const auto   e             = engine();
    const double line_spacing  = this->line_spacing();
    const double block_spacing = this->block_spacing();

    const double h_slack   = 6 * 0.75;
    const double margin    = qMin(4.0, block_spacing);
    const double min_y_pos = -e->line_height - h_slack;
    double       max_y_pos = -e->line_height - h_slack - margin;
    for (auto block : e->active_blocks) {
        max_y_pos += block->lines.size() * line_spacing + block_spacing;
    }
    if (!e->is_empty()) { max_y_pos -= block_spacing; }

    const int top    = qCeil(min_y_pos);
    const int bottom = qMax(top + 1, qFloor(max_y_pos));

    return ViewBounds{
        .top    = top,
        .bottom = bottom,
    };
}

void PrimaryEditor::request_scroll(double delta) {
    request_scroll_to_pos(context()->viewport_y_pos + delta);
}

void PrimaryEditor::request_scroll_to_pos(double pos) {
    auto ctx                 = context();
    const auto [top, bottom] = recalc_view_bounds();
    target_scroll_pos_       = qBound<double>(top, pos, bottom);
    request_update(false);
}

void PrimaryEditor::process_scroll_request() {
    auto ctx = context();
    if (qAbs(ctx->viewport_y_pos - target_scroll_pos_) < 1) { return; }
    ctx->scroll_to(qFloor(target_scroll_pos_));
}

int PrimaryEditor::horizontal_margin() const {
    return text_view_margins_.left() + text_view_margins_.right();
}

int PrimaryEditor::horizontal_margin_hint() const {
    return 8;
}

void PrimaryEditor::del(int times) {
    execute_delete_action(times);
    emit on_text_change(context_->edit_text);
    request_update(true);
}

void PrimaryEditor::insert(const QString &text, bool batch_mode) {
    const auto ctx = context();
    const auto e   = engine();
    Q_ASSERT(e->is_cursor_available());

    if (ctx->has_sel()) {
        Q_ASSERT(!e->preedit);
        execute_delete_action(0);
    } else if (e->preedit) {
        ctx->commit_preedit();
    }
    if (batch_mode) {
        bool filtered = false;
        if (insert_filter_enabled()) { filtered = insert_action_filter(text, true); }
        if (!filtered) {
            execute_insert_action(text, true);
            if (!text.isEmpty()) { ctx->cursor_moved = true; }
        }
    } else {
        const auto block_text  = e->current_block()->text();
        const auto insert_pos  = e->cursor.pos;
        const auto text_before = block_text.left(insert_pos);
        const auto text_after  = block_text.right(block_text.length() - insert_pos);
        const auto text_in     = restrict_rule()->restrict(text, text_before, text_after);

        bool filtered = false;
        if (insert_filter_enabled()) { filtered = insert_action_filter(text_in, false); }

        if (!filtered) { execute_insert_action(text_in, false); }
    }

    emit on_text_change(ctx->edit_text);
    request_update(false);
}

void PrimaryEditor::select(int start_pos, int end_pos) {
    const auto ctx = context();
    auto      &sel = ctx->sel;

    sel.clear();
    sel.from = qBound(0, start_pos, ctx->edit_text.length());
    sel.to   = qBound(0, end_pos, ctx->edit_text.length());

    ctx->cached_render_data_ready = false;
    request_update(false);
}

void PrimaryEditor::unselect() {
    context()->unset_sel();
    request_update(false);
}

void PrimaryEditor::move(int offset, bool extend_sel) {
    const int last_block = engine()->active_block_index;
    context()->move(offset, extend_sel);
    if (const int this_block = engine()->active_block_index; last_block != this_block) {
        internal_action_on_leave_block(last_block);
    }
    request_update(true);
}

void PrimaryEditor::move_to(int pos, bool extend_sel) {
    const int last_block = engine()->active_block_index;
    context()->move_to(pos, extend_sel);
    if (const int this_block = engine()->active_block_index; last_block != this_block) {
        internal_action_on_leave_block(last_block);
    }
    request_update(true);
}

void PrimaryEditor::vertical_move(bool up) {
    const int last_block = engine()->active_block_index;
    context()->vertical_move(up);
    if (const int this_block = engine()->active_block_index; last_block != this_block) {
        internal_action_on_leave_block(last_block);
    }
    request_update(true);
}

void PrimaryEditor::copy() {
    auto       clipboard = QApplication::clipboard();
    const auto ctx       = context();
    const auto e         = engine();
    if (ctx->has_sel()) {
        const int   pos_from = qMin(ctx->sel.from, ctx->sel.to);
        const int   pos_to   = qMax(ctx->sel.from, ctx->sel.to);
        const auto  loc_from = ctx->get_textloc_at_pos(pos_from, -1);
        const auto  loc_to   = ctx->get_textloc_at_pos(pos_to, 1);
        QStringList copied_text{};
        for (int index = loc_from.block_index; index <= loc_to.block_index; ++index) {
            const auto block = e->active_blocks[index];
            const int  from  = index == loc_from.block_index ? loc_from.pos : 0;
            const int  to    = index == loc_to.block_index ? loc_to.pos : block->text_len();
            copied_text << block->text().mid(from, to - from).toString();
        }
        clipboard->setText(copied_text.join('\n'));
    } else if (e->is_cursor_available()) {
        //! copy the current block if has no sel
        const auto copied_text = e->current_block()->text();
        clipboard->setText(copied_text.toString());
    }
}

void PrimaryEditor::cut() {
    copy();
    if (context()->has_sel()) {
        del(0);
    } else {
        //! cut the current block if has no sel, also remove the block
        context()->move(-engine()->cursor.pos, false);
        del(engine()->current_block()->text_len() + 1);
    }
}

void PrimaryEditor::paste() {
    auto clipboard = QApplication::clipboard();
    auto mime      = clipboard->mimeData();
    if (!mime->hasText()) { return; }
    insert(clipboard->text(), true);
}

void PrimaryEditor::undo() {
    if (auto opt = history()->get_undo_action()) {
        auto action = opt.value();
        context()->unset_sel();
        set_caret_loc(action.loc);
        switch (action.type) {
            case TextEditAction::Insert: {
                direct_batch_insert(action.text);
            } break;
            case TextEditAction::Delete: {
                direct_delete(action.text.length(), nullptr);
            } break;
        }
        emit on_text_change(context()->edit_text);
        request_update(true);
    }
}

void PrimaryEditor::redo() {
    if (auto opt = history()->get_redo_action()) {
        auto action = opt.value();
        context()->unset_sel();
        set_caret_loc(action.loc);
        switch (action.type) {
            case TextEditAction::Insert: {
                direct_batch_insert(action.text);
            } break;
            case TextEditAction::Delete: {
                direct_delete(action.text.length(), nullptr);
            } break;
        }
        emit on_text_change(context()->edit_text);
        request_update(true);
    }
}

bool PrimaryEditor::has_default_tic_handler(TextInputCommand cmd) {
    return cmd == TextInputCommand::Reject || cmd == TextInputCommand::InsertPrintable
        || default_tic_handlers_.contains(cmd);
}

void PrimaryEditor::execute_text_input_command(TextInputCommand cmd, const QString &text) {
    static QSet<TextInputCommand> READONLY_COMMANDS{
        TextInputCommand::Cancel,
        TextInputCommand::Copy,
        TextInputCommand::ScrollUp,
        TextInputCommand::ScrollDown,
        TextInputCommand::MoveToPrevPage,
        TextInputCommand::MoveToNextPage,
        TextInputCommand::MoveToStartOfDocument,
        TextInputCommand::MoveToEndOfDocument,
        TextInputCommand::SelectAll,
    };

    if (readonly() && !READONLY_COMMANDS.contains(cmd)) { return; }

    if (cmd == TextInputCommand::InsertPrintable) {
        insert(text, false);
    } else if (default_tic_handlers_.contains(cmd)) {
        auto handler = default_tic_handlers_.value(cmd);
        std::invoke(handler, this);
    }
}

bool PrimaryEditor::test_pos_in_sel_area(const QPoint &pos) const {
    const auto ctx = context();
    const auto e   = engine();
    if (!ctx->has_sel()) { return false; }
    const auto loc = ctx->get_textloc_at_rel_vpos(pos, false);
    if (loc.block_index == -1) { return false; }
    const auto  block    = e->active_blocks[loc.block_index];
    const auto &line     = block->lines[loc.row];
    const int   text_pos = e->text_pos_at_text_loc(loc);
    const auto &sel      = ctx->sel;
    Q_ASSERT(!block->is_dirty());
    if (!(text_pos >= qMin(sel.from, sel.to) && text_pos <= qMax(sel.from, sel.to))) {
        return false;
    }
    const double leading_space = line.is_first_line() ? e->standard_char_width * 2 : 0;
    if (const double x = pos.x() - leading_space; !(x >= 0 && x <= line.cached_text_width)) {
        return false;
    }
    return true;
}

void PrimaryEditor::draw(QPainter *p) {
    if (enabled()) {
        draw_selection(p);
        draw_text(p);
        if (!readonly()) { draw_caret(p); }
    }
}

void PrimaryEditor::draw_text(QPainter *p) {
    const auto  ctx = context();
    const auto  e   = engine();
    const auto &d   = context_->cached_render_state;

    if (!d.found_visible_block) { return; }

    const auto pal   = palette();
    const auto flags = Qt::AlignBaseline | Qt::TextDontClip;

    p->save();
    p->setPen(pal.color(QPalette::Text));

    const double indent        = e->standard_char_width * 2;
    const double line_spacing  = this->line_spacing();
    const double block_spacing = this->block_spacing();
    const auto   viewport      = text_area();

    QRectF bb(viewport.left(), viewport.top(), viewport.width(), e->line_height);
    bb.translate(0, d.first_visible_block_y_pos - ctx->viewport_y_pos);

    const auto y_pos = d.first_visible_block_y_pos - ctx->viewport_y_pos;
    auto       pos   = viewport.topLeft() + QPointF(0, y_pos);

    for (int index = d.visible_block.first; index <= d.visible_block.last; ++index) {
        const auto block = e->active_blocks[index];

        for (const auto &line : block->lines) {
            const double leading_space = line.is_first_line() ? indent : 0;
            const double spacing       = line.char_spacing();
            bb.setLeft(viewport.left() + leading_space);
            for (const auto c : line.text()) {
                p->drawText(bb, flags, c);
                const double advance = e->fm.horizontalAdvance(c);
                bb.adjust(advance + spacing, 0, 0, 0);
            }
            bb.translate(0, line_spacing);
        }

        bb.translate(0, block_spacing);
    }

    p->restore();
}

void PrimaryEditor::draw_selection(QPainter *p) {
    const auto  ctx = context();
    const auto  e   = engine();
    const auto &d   = ctx->cached_render_state;

    if (!ctx->has_sel()) { return; }
    if (d.visible_sel.first == -1) { return; }

    const auto pal = palette();

    const auto   viewport     = text_area();
    const double line_spacing = this->line_spacing();

    double y_pos = d.cached_block_y_pos[d.visible_sel.first];

    for (int index = d.visible_sel.first; index <= d.visible_sel.last; ++index) {
        const auto   block         = e->active_blocks[index];
        const bool   is_first      = index == d.sel_loc_from.block_index;
        const bool   is_last       = index == d.sel_loc_to.block_index;
        const int    line_nr_begin = is_first ? d.sel_loc_from.row : 0;
        const int    line_nr_end   = is_last ? d.sel_loc_to.row : block->lines.size() - 1;
        const double stride        = line_nr_begin * line_spacing;

        double y_pos = d.cached_block_y_pos[index] + stride - context_->viewport_y_pos;
        //! add the minus part to make the selection area align center
        y_pos -= e->fm.descent() * 0.5;

        for (int line_nr = line_nr_begin; line_nr <= line_nr_end; ++line_nr) {
            const auto line   = block->lines[line_nr];
            const auto offset = line.is_first_line() ? e->standard_char_width * 2 : 0;

            double x_pos = offset;
            if (is_first && line_nr == d.sel_loc_from.row) {
                x_pos += line.char_spacing() * d.sel_loc_from.col;
                for (const auto &c : line.text().left(d.sel_loc_from.col)) {
                    x_pos += e->fm.horizontalAdvance(c);
                }
            }

            double sel_width = line.cached_text_width + (line.text_len() - 1) * line.char_spacing()
                             - (x_pos - offset);
            if (is_last && line_nr == d.sel_loc_to.row) {
                sel_width -= line.char_spacing() * (line.text_len() - d.sel_loc_to.col);
                for (const auto &c : line.text().mid(d.sel_loc_to.col)) {
                    sel_width -= e->fm.horizontalAdvance(c);
                }
            }

            QRectF bb(x_pos, y_pos, sel_width, e->line_height);
            bb.translate(viewport.topLeft());
            p->fillRect(bb, pal.highlight());

            y_pos += line_spacing;
        }
    }
}

void PrimaryEditor::draw_caret(QPainter *p) {
    const auto  ctx = context();
    const auto  e   = engine();
    const auto &d   = ctx->cached_render_state;

    Q_ASSERT(ctx->cached_render_data_ready);

    if (!caret_blink_.enabled) { return; }
    if (!e->is_cursor_available()) { return; }
    if (!d.active_block_visible) { return; }

    const auto &caret_loc = text_drag_state_.caret_loc;

    int block_index = -1;
    int row         = -1;
    int col         = -1;
    if (text_drag_state_.enabled) {
        row         = caret_loc.row;
        col         = caret_loc.col;
        block_index = caret_loc.block_index;
    } else {
        row         = e->cursor.row;
        col         = e->cursor.col;
        block_index = e->active_block_index;
    }

    const auto   viewport     = text_area();
    const double line_spacing = this->line_spacing();

    Q_ASSERT(block_index >= d.visible_block.first && block_index <= d.visible_block.last);
    const double y_pos = d.cached_block_y_pos[block_index] + row * line_spacing;

    const auto  &line          = e->active_blocks[block_index]->lines[row];
    const double leading_space = line.is_first_line() ? e->standard_char_width * 2 : 0;
    double       cursor_x_pos  = leading_space + line.char_spacing() * col;
    for (const auto &c : line.text().left(col)) { cursor_x_pos += e->fm.horizontalAdvance(c); }
    const double cursor_y_pos = y_pos - context_->viewport_y_pos;
    const auto   cursor_pos   = QPoint(cursor_x_pos, cursor_y_pos) + viewport.topLeft();

    p->save();

    auto      caret_color      = palette().color(QPalette::Text);
    const int caret_x_pos      = cursor_pos.x();
    const int caret_y_pos_from = cursor_pos.y();
    const int caret_y_pos_to   = caret_y_pos_from + e->fm.height();

    if (text_drag_state_.enabled) {
        p->setPen(QPen(caret_color, 0.8, Qt::DotLine));
        p->drawLine(caret_x_pos, caret_y_pos_from, caret_x_pos, caret_y_pos_to);
    } else {
        switch (caret_blink_.style) {
            case CaretBlinkStyle::Solid: {
                p->setPen(QPen(caret_color, 0.8));
                p->drawLine(caret_x_pos, caret_y_pos_from, caret_x_pos, caret_y_pos_to);
            } break;
            case CaretBlinkStyle::Blink: {
                if (caret_blink_.progress < 0.5) {
                    p->setPen(QPen(caret_color, 0.8));
                    p->drawLine(caret_x_pos, caret_y_pos_from, caret_x_pos, caret_y_pos_to);
                }
            } break;
            case CaretBlinkStyle::Expand: {
                const double k           = qBound(0.0, qAbs(caret_blink_.progress * 3 - 1.5), 1.0);
                const double full_height = caret_y_pos_to - caret_y_pos_from;
                const double height      = full_height * k;
                const int    y0          = caret_y_pos_from + (full_height - height) / 2;
                const int    y1          = y0 + height;
                p->setPen(QPen(caret_color, 0.8));
                p->drawLine(caret_x_pos, y0, caret_x_pos, y1);
            } break;
            case CaretBlinkStyle::Smooth: {
                caret_color.setAlphaF(qAbs(caret_blink_.progress * 2 - 1));
                p->setPen(QPen(caret_color, 0.8));
                p->drawLine(caret_x_pos, caret_y_pos_from, caret_x_pos, caret_y_pos_to);
            } break;
            case CaretBlinkStyle::Phase: {
                caret_color.setAlphaF((qCos(caret_blink_.progress * M_PI * 2) + 1) / 2);
                p->setPen(QPen(caret_color, 0.8));
                p->drawLine(caret_x_pos, caret_y_pos_from, caret_x_pos, caret_y_pos_to);
            } break;
            case CaretBlinkStyle::Stack: {
                const double t = caret_blink_.progress;
                const double h = caret_y_pos_to - caret_y_pos_from;
                if (t < 0.2) {
                    p->setPen(QPen(caret_color, 0.8));
                    p->drawLine(caret_x_pos, caret_y_pos_from, caret_x_pos, caret_y_pos_to);
                } else if (t < 0.4) {
                    const double max_dt = 0.2;
                    const double dt     = qBound(0.0, t - 0.2, max_dt);
                    const double k      = h / (max_dt * max_dt);
                    const int    dh     = qBound<int>(0, k * dt * dt, h);
                    const int    y0     = caret_y_pos_from + h - dh;
                    p->setPen(QPen(caret_color, 0.8));
                    p->drawLine(caret_x_pos, caret_y_pos_from, caret_x_pos, y0);
                    p->setPen(QPen(caret_color, 0.8, Qt::DotLine));
                    p->drawLine(caret_x_pos, y0, caret_x_pos, caret_y_pos_to);
                } else {
                    p->setPen(QPen(caret_color, 0.8));
                    const double max_dt = 0.6;
                    const double dt     = qBound(0.0, t - 0.4, max_dt);
                    const int    dh     = h * dt / max_dt;
                    const int    y1     = caret_y_pos_from + h - dh;
                    const int    y0     = qBound(caret_y_pos_from, y1 - dh, caret_y_pos_to);
                    p->setPen(QPen(caret_color, 0.8, Qt::DotLine));
                    p->drawLine(caret_x_pos, y0, caret_x_pos, y1);
                    p->setPen(QPen(caret_color, 0.8));
                    p->drawLine(caret_x_pos, y1, caret_x_pos, caret_y_pos_to);
                }
            } break;
        }
    }

    p->restore();
}

VisualTextEditContext *PrimaryEditor::context() const {
    Q_ASSERT(context_);
    return context_.get();
}

TextViewEngine *PrimaryEditor::engine() const {
    return std::addressof(context()->engine);
}

TextEditHistory *PrimaryEditor::history() const {
    Q_ASSERT(history_);
    return history_.get();
}

bool PrimaryEditor::insert_action_filter(const QString &text, bool batch_mode) {
    return false;
}

void PrimaryEditor::internal_action_on_enter_block(int block_index) {}

void PrimaryEditor::internal_action_on_leave_block(int block_index) {}

void PrimaryEditor::direct_remove_sel(QString *deleted_text) {
    if (auto ctx = context(); ctx->has_sel()) {
        ctx->remove_sel_region(deleted_text);
    } else {
        Q_UNREACHABLE();
    }
}

void PrimaryEditor::execute_delete_action(int times) {
    Q_ASSERT(!engine()->preedit);
    Q_ASSERT(engine()->is_cursor_available());

    QString deleted_text{};
    if (context()->has_sel()) {
        direct_remove_sel(&deleted_text);
    } else {
        direct_delete(times, &deleted_text);
    }

    const auto loc = caret_loc();
    history()->push(TextEditAction::from_action(TextEditAction::Type::Delete, loc, deleted_text));
}

void PrimaryEditor::execute_insert_action(const QString &text, bool batch_mode) {
    Q_ASSERT(!engine()->preedit);
    Q_ASSERT(engine()->is_cursor_available());
    Q_ASSERT(!context()->has_sel());

    //! ATTENTION: save the loc before insertion, do not move the statement!
    const auto loc = caret_loc();

    if (batch_mode) {
        direct_batch_insert(text);
    } else {
        direct_insert(text);
    }

    history()->push(TextEditAction::from_action(TextEditAction::Type::Insert, loc, text));
}

void PrimaryEditor::direct_delete(int times, QString *deleted_text) {
    if (auto ctx = context(); !ctx->has_sel()) {
        ctx->del(times, false, deleted_text);
    } else {
        Q_UNREACHABLE();
    }
}

void PrimaryEditor::direct_insert(const QString &text) {
    Q_ASSERT(!engine()->preedit);
    Q_ASSERT(!context()->has_sel());
    context()->insert(text);
}

void PrimaryEditor::direct_batch_insert(const QString &multiline_text) {
    Q_ASSERT(!engine()->preedit);
    Q_ASSERT(!context()->has_sel());

    auto lines = multiline_text.split('\n');
    direct_insert(lines.first());
    for (int i = 1; i < lines.size(); ++i) {
        engine()->break_block_at_cursor_pos();
        direct_insert(lines[i]);
    }

    //! NOTE: a single newline will not dive into the insert() fncall, mark as cursor-moved
    //! mannually
    context()->cursor_moved             = true;
    context()->cached_render_data_ready = false;
}

void PrimaryEditor::request_update(bool sync) {
    if (context()->cursor_moved) {
        caret_timer_->stop();
        caret_blink_.progress = 0.0;
        caret_timer_->start();
    }
    update_requested_ = true;
    if (sync) {
        prerender_timer_->stop();
        handle_prerender_timer_on_timeout();
        prerender_timer_->start();
    }
}

void PrimaryEditor::save_and_set_cursor(Qt::CursorShape cursor) {
    //! FIXME: save and set cursor
    setCursor(cursor);
}

void PrimaryEditor::restore_cursor() {
    //! FIXME: restore cursor
    setCursor(Qt::ArrowCursor);
}

void PrimaryEditor::complete_text_drag() {
    const auto ctx = context();
    const auto e   = engine();

    Q_ASSERT(text_drag_state_.enabled);
    Q_ASSERT(ctx->has_sel());
    Q_ASSERT(e->is_cursor_available());
    Q_ASSERT(!e->preedit);

    const auto &sel       = ctx->sel;
    const int   sel_left  = qMin(sel.from, sel.to);
    const int   sel_right = qMax(sel.from, sel.to);
    const int   len       = sel_right - sel_left;
    const auto &loc       = text_drag_state_.caret_loc;
    const int   pos       = e->text_pos_at_text_loc(loc);

    if (!(pos >= sel_left && pos <= sel_right)) {
        const auto sel_left_loc  = ctx->get_textloc_at_pos(sel_left, -1);
        const auto sel_right_loc = ctx->get_textloc_at_pos(sel_right, 1);

        auto      caret_loc = loc;
        const int last_pos  = e->text_pos_at_text_loc(caret_loc);

        const bool should_recalc_loc =
            caret_loc.block_index == sel_right_loc.block_index && last_pos >= sel_right;

        //! TODO: add new action for moving the text

        del(0);
        const auto text = history()->current_record().text;

        if (should_recalc_loc) {
            caret_loc = caret_loc = ctx->get_textloc_at_pos(last_pos - len, -1);
        }

        set_caret_loc(caret_loc);
        insert(text, true);

        const int pos = e->current_block()->text_pos + this->caret_loc().pos;
        select(pos - len, pos);
    }

    text_drag_state_.enabled = false;
}

void PrimaryEditor::reset_mouse_action_state() {
    switch (ma_state_) {
        case MouseActionState::Idle: {
        } break;
        case MouseActionState::PreAutoScroll: {
            auto_scroll_info_.enabled = false;
        } break;
        case MouseActionState::AutoScroll: {
            auto_scroll_info_.enabled = false;
        } break;
        case MouseActionState::LightAutoScroll: {
            auto_scroll_info_.enabled = false;
        } break;
        case MouseActionState::SetCaret: {
            Q_UNREACHABLE();
        } break;
        case MouseActionState::PreSelectText: {
        } break;
        case MouseActionState::SelectText: {
        } break;
        case MouseActionState::PreDragSelection: {
        } break;
        case MouseActionState::DragSelection: {
            text_drag_state_.enabled = false;
        } break;
        case MouseActionState::DragSelectionDone: {
            Q_UNREACHABLE();
        } break;
    }
    ma_state_ = MouseActionState::Idle;
}

void PrimaryEditor::prepare_canvas() {
    if (const auto size = this->size(); canvas_.isNull()) {
        canvas_ = QPixmap(size);
        canvas_.fill(palette().color(backgroundRole()));
    } else if (size != canvas_.size()) {
        canvas_ = canvas_.scaled(size);
    }
}

void PrimaryEditor::reset_input_method_state() {
    const bool on = enabled() && !readonly();
    setAttribute(Qt::WA_InputMethodEnabled, on);
}

void PrimaryEditor::handle_tic_insert_new_line() {
    insert("\n", true);
}

void PrimaryEditor::handle_tic_insert_before_block() {
    auto e = engine();
    e->cursor.reset();
    e->break_block_at_cursor_pos();
    move(-1, false);
}

void PrimaryEditor::handle_tic_insert_after_block() {
    auto       e      = engine();
    const auto block  = e->current_block();
    auto      &cursor = e->cursor;
    cursor.row        = block->lines.size() - 1;
    cursor.pos        = block->text_len();
    cursor.col        = cursor.pos - block->lines[cursor.row].text_offset();
    insert("\n", true);
}

void PrimaryEditor::handle_tic_cancel() {
    unselect();
}

void PrimaryEditor::handle_tic_undo() {
    undo();
}

void PrimaryEditor::handle_tic_redo() {
    redo();
}

void PrimaryEditor::handle_tic_copy() {
    copy();
}

void PrimaryEditor::handle_tic_cut() {
    cut();
}

void PrimaryEditor::handle_tic_paste() {
    paste();
}

void PrimaryEditor::handle_tic_scroll_up() {
    request_scroll(-line_spacing());
}

void PrimaryEditor::handle_tic_scroll_down() {
    request_scroll(line_spacing());
}

void PrimaryEditor::handle_tic_move_to_prev_char() {
    move(-1, false);
}

void PrimaryEditor::handle_tic_move_to_next_char() {
    move(1, false);
}

void PrimaryEditor::handle_tic_move_to_prev_word() {
    const auto  e      = engine();
    const auto &cursor = e->cursor;
    const auto  block  = e->current_block();
    const int   len    = cursor.pos;
    if (len == 0) {
        move(-1, false);
    } else {
        const auto word   = tokenizer()->get_last_word(block->text().left(len).toString());
        const int  offset = word.length();
        Q_ASSERT(offset <= cursor.pos);
        move_to(block->text_pos + cursor.pos - offset, false);
    }
}

void PrimaryEditor::handle_tic_move_to_next_word() {
    const auto  e      = engine();
    const auto &cursor = e->cursor;
    const auto  block  = e->current_block();
    const int   len    = block->text_len() - cursor.pos;
    if (len == 0) {
        move(1, false);
    } else {
        const auto word   = tokenizer()->get_first_word(block->text().right(len).toString());
        const int  offset = word.length();
        Q_ASSERT(offset <= len);
        move_to(block->text_pos + cursor.pos + offset, false);
    }
}

void PrimaryEditor::handle_tic_move_to_prev_line() {
    vertical_move(true);
}

void PrimaryEditor::handle_tic_move_to_next_line() {
    vertical_move(false);
}

void PrimaryEditor::handle_tic_move_to_start_of_line() {
    const auto block = engine()->current_block();
    const auto line  = block->current_line();
    move_to(block->text_pos + line.text_offset(), false);
}

void PrimaryEditor::handle_tic_move_to_end_of_line() {
    const auto block = engine()->current_block();
    const auto line  = block->current_line();
    const auto pos   = block->text_pos + line.text_offset() + line.text().length();
    move_to(pos, false);
}

void PrimaryEditor::handle_tic_move_to_start_of_block() {
    move_to(engine()->current_block()->text_pos, false);
}

void PrimaryEditor::handle_tic_move_to_end_of_block() {
    const auto block = engine()->current_block();
    move_to(block->text_pos + block->text_len(), false);
}

void PrimaryEditor::handle_tic_move_to_start_of_document() {
    move_to(0, false);
    request_scroll_to_pos(recalc_view_bounds().top);
}

void PrimaryEditor::handle_tic_move_to_end_of_document() {
    move_to(engine()->text_ref->length(), false);
    request_scroll_to_pos(recalc_view_bounds().bottom);
}

void PrimaryEditor::handle_tic_move_to_prev_page() {
    request_scroll(-text_area().height() * 0.5);
}

void PrimaryEditor::handle_tic_move_to_next_page() {
    request_scroll(text_area().height() * 0.5);
}

void PrimaryEditor::handle_tic_move_to_prev_block() {
    if (const auto e = engine(); e->active_block_index > 0) {
        const auto block      = e->current_block();
        const auto prev_block = e->active_blocks[e->active_block_index - 1];
        move(prev_block->text_pos - block->text_pos - e->cursor.pos - 1, false);
    }
}

void PrimaryEditor::handle_tic_move_to_next_block() {
    if (const auto e = engine(); e->active_block_index + 1 < e->active_blocks.size()) {
        const auto block      = e->current_block();
        const auto next_block = e->active_blocks[e->active_block_index + 1];
        move(next_block->text_pos - block->text_pos - e->cursor.pos + 1, false);
    }
}

void PrimaryEditor::handle_tic_delete_prev_char() {
    del(-1);
}

void PrimaryEditor::handle_tic_delete_next_char() {
    del(1);
}

void PrimaryEditor::handle_tic_delete_prev_word() {
    const auto  e      = engine();
    const auto &cursor = e->cursor;
    const auto  block  = e->current_block();
    const int   len    = cursor.pos;
    if (context_->has_sel() || len == 0) {
        del(-1);
    } else {
        const auto word   = tokenizer()->get_last_word(block->text().left(len).toString());
        const int  offset = word.length();
        Q_ASSERT(offset <= cursor.pos);
        del(-offset);
    }
}

void PrimaryEditor::handle_tic_delete_next_word() {
    const auto  e      = engine();
    const auto &cursor = e->cursor;
    const auto  block  = e->current_block();
    const int   len    = block->text_len() - cursor.pos;
    if (context_->has_sel() || len == 0) {
        del(1);
    } else {
        const auto word   = tokenizer()->get_first_word(block->text().right(len).toString());
        const int  offset = word.length();
        Q_ASSERT(offset <= len);
        del(offset);
    }
}

void PrimaryEditor::handle_tic_delete_to_start_of_line() {
    const auto  e      = engine();
    const auto &cursor = e->cursor;
    const auto &block  = e->current_block();
    int         times  = cursor.col;
    if (times == 0) {
        if (cursor.row > 0) {
            times = block->len_of_line(cursor.row - 1);
        } else {
            times = 1;
        }
    }
    del(-times);
}

void PrimaryEditor::handle_tic_delete_to_end_of_line() {
    const auto  e      = engine();
    const auto &cursor = e->cursor;
    const auto &block  = e->current_block();
    int         times  = block->len_of_line(cursor.row) - cursor.col;
    if (times == 0) {
        if (cursor.row + 1 == block->lines.size()) {
            times = 1;
        } else {
            times = block->len_of_line(cursor.row + 1);
        }
    }
    del(times);
}

void PrimaryEditor::handle_tic_select_prev_char() {
    move(-1, true);
}

void PrimaryEditor::handle_tic_select_next_char() {
    move(1, true);
}

void PrimaryEditor::handle_tic_select_prev_word() {
    const auto  e      = engine();
    const auto &cursor = e->cursor;
    const auto &block  = e->current_block();
    const int   len    = cursor.pos;
    if (len == 0) {
        move(-1, true);
    } else {
        const auto word   = tokenizer()->get_last_word(block->text().left(len).toString());
        const int  offset = word.length();
        Q_ASSERT(offset <= cursor.pos);
        move_to(block->text_pos + cursor.pos - offset, true);
    }
}

void PrimaryEditor::handle_tic_select_next_word() {
    const auto  e      = engine();
    const auto &cursor = e->cursor;
    const auto &block  = e->current_block();
    const int   len    = block->text_len() - cursor.pos;
    if (len == 0) {
        move(1, true);
    } else {
        const auto word   = tokenizer()->get_first_word(block->text().right(len).toString());
        const int  offset = word.length();
        Q_ASSERT(offset <= len);
        move_to(block->text_pos + cursor.pos + offset, true);
    }
}

void PrimaryEditor::handle_tic_select_to_prev_line() {
    const auto ctx      = context();
    const int  sel_from = ctx->has_sel() ? ctx->sel.from : ctx->edit_cursor_pos;
    if (ctx->has_sel()) { ctx->unset_sel(); }
    vertical_move(true);
    select(sel_from, ctx->edit_cursor_pos);
}

void PrimaryEditor::handle_tic_select_to_next_line() {
    const auto ctx      = context();
    const int  sel_from = ctx->has_sel() ? ctx->sel.from : ctx->edit_cursor_pos;
    if (ctx->has_sel()) { ctx->unset_sel(); }
    vertical_move(false);
    select(sel_from, ctx->edit_cursor_pos);
}

void PrimaryEditor::handle_tic_select_to_start_of_line() {
    move(-engine()->cursor.col, true);
}

void PrimaryEditor::handle_tic_select_to_end_of_line() {
    const auto e = engine();
    move(e->current_line().text().length() - e->cursor.col, true);
}

void PrimaryEditor::handle_tic_select_to_start_of_block() {
    move_to(engine()->current_block()->text_pos, true);
}

void PrimaryEditor::handle_tic_select_to_end_of_block() {
    const auto block = engine()->current_block();
    move_to(block->text_pos + block->text_len(), true);
}

void PrimaryEditor::handle_tic_select_block() {
    const auto block = engine()->current_block();
    select(block->text_pos, block->text_pos + block->text_len());
}

void PrimaryEditor::handle_tic_select_prev_page() {
    const auto   origin = context_->get_vpos_at_cursor();
    const QPoint dest(
        origin.x(), origin.y() - context_->viewport_y_pos - context_->viewport_height);
    const auto dest_loc = context_->get_textloc_at_rel_vpos(dest, false);
    const int  pos      = engine()->text_pos_at_text_loc(dest_loc);
    move_to(pos, true);
}

void PrimaryEditor::handle_tic_select_next_page() {
    const auto   origin = context_->get_vpos_at_cursor();
    const QPoint dest(
        origin.x(), origin.y() - context_->viewport_y_pos + context_->viewport_height);
    const auto dest_loc = context_->get_textloc_at_rel_vpos(dest, false);
    const int  pos      = engine()->text_pos_at_text_loc(dest_loc);
    move_to(pos, true);
}

void PrimaryEditor::handle_tic_select_to_start_of_doc() {
    move_to(0, true);
}

void PrimaryEditor::handle_tic_select_to_end_of_doc() {
    move_to(engine()->text_ref->length(), true);
}

void PrimaryEditor::handle_tic_select_all() {
    select(0, engine()->text_ref->length());
}

void PrimaryEditor::update_text_view_geometry() {
    const int margin       = horizontal_margin_hint();
    const int left_margin  = margin / 2;
    const int right_margin = margin - left_margin;
    text_view_margins_     = QMargins(left_margin, 4, right_margin, 4);
    emit on_text_area_change(text_area());
}

void PrimaryEditor::render() {
    auto ctx = context();
    ctx->prepare_render_data();

    //! TODO: scroll to cursor

    ctx->cursor_moved = false;
    update();
}

void PrimaryEditor::handle_on_text_area_change(const QRect &rect) {
    context()->resize_viewport(rect.width(), rect.height());
    request_update(true);
}

void PrimaryEditor::handle_caret_timer_on_timeout() {
    Q_ASSERT(caret_blink_.interval == caret_timer_->interval());
    if (!caret_blink_.enabled) { return; }
    const double step     = caret_blink_.interval * 1.0 / caret_blink_.period;
    caret_blink_.progress = fmod(caret_blink_.progress + step, 1.0);
    request_update(false);
}

void PrimaryEditor::handle_prerender_timer_on_timeout() {
    if (auto_scroll_info_.enabled) {
        const double delta = (auto_scroll_info_.ref_pos - auto_scroll_info_.base_pos) / 10;
        request_scroll(delta);
    }

    process_scroll_request();

    if (update_requested_) {
        emit on_render_request();
        update_requested_ = false;
    }
}

void PrimaryEditor::handle_on_activate() {
    if (auto e = engine(); e->is_empty()) {
        e->insert_block(0);
        e->active_block_index = 0;
        e->cursor.reset();
        context()->cursor_moved = true;
        request_update(true);
    }
}

PrimaryEditor::PrimaryEditor(QWidget *parent)
    : QWidget(parent) {
    init();
}

QSize PrimaryEditor::minimumSizeHint() const {
    const int  min_text_line_chars = 8;
    const auto e                   = engine();
    const auto margins             = contentsMargins() + text_view_margins_;
    const auto line_spacing        = this->line_spacing();
    const auto hori_margin         = margins.left() + margins.right();
    const auto vert_margin         = margins.top() + margins.bottom();
    const auto min_width           = min_text_line_chars * e->standard_char_width + hori_margin;
    const auto min_height          = line_spacing * 3 + e->block_spacing * 2 + vert_margin;
    return QSize(min_width, min_height);
}

QSize PrimaryEditor::sizeHint() const {
    return minimumSizeHint();
}

void PrimaryEditor::init() {
    Q_UNIMPLEMENTED();

    //! config widget properties
    setFocusPolicy(Qt::ClickFocus);
    setAutoFillBackground(true);
    setAcceptDrops(true);
    setMouseTracking(true);
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    setContentsMargins({});

    //! editor context
    const auto text_area = this->text_area();
    text_font_           = this->font();
    context_.reset(new VisualTextEditContext(QFontMetrics(text_font_), text_area.width()));
    context_->resize_viewport(context_->viewport_width, text_area.height());
    request_scroll_to_pos(recalc_view_bounds().top);

    //! text edit history
    history_.reset(new TextEditHistory);

    //! mouse action state
    ma_state_ = MouseActionState::Idle;

    //! text drag info
    text_drag_state_.enabled = false;

    //! default text input command handlers
    init_default_tic_handlers();

    //! scroll info
    auto_scroll_info_.enabled = false;

    //! insert action filter
    set_insert_filter_enabled(false);

    //! caret blink info
    caret_blink_.enabled  = true;
    caret_blink_.style    = CaretBlinkStyle::Blink;
    caret_blink_.period   = 1000;
    caret_blink_.interval = 50;
    caret_blink_.progress = 0.0;

    caret_timer_ = new QTimer(this);
    caret_timer_->setInterval(caret_blink_.interval);
    caret_timer_->setSingleShot(false);

    //! prereender info
    prerender_timer_ = new QTimer(this);
    prerender_timer_->setInterval(16);
    prerender_timer_->setSingleShot(false);

    //! singal & slot
    connect(
        this,
        &PrimaryEditor::on_text_area_change,
        this,
        &PrimaryEditor::handle_on_text_area_change);
    connect(caret_timer_, &QTimer::timeout, this, &PrimaryEditor::handle_caret_timer_on_timeout);
    connect(
        prerender_timer_,
        &QTimer::timeout,
        this,
        &PrimaryEditor::handle_prerender_timer_on_timeout);
    connect(this, &PrimaryEditor::on_render_request, this, &PrimaryEditor::render);
    connect(this, &PrimaryEditor::on_activate, this, &PrimaryEditor::handle_on_activate);

    //! finalize

    //! NOTE: call it before any rendering
    prepare_canvas();

    set_enabled(false);
    set_readonly(false);

    caret_timer_->start();
    prerender_timer_->start();
}

void PrimaryEditor::init_default_tic_handlers() {
    default_tic_handlers_.clear();

#define PUT_DEFAULT_TIC_HANDLER(command, handler) \
    default_tic_handlers_.insert(TextInputCommand::command, &PrimaryEditor::handle_tic_##handler)

    PUT_DEFAULT_TIC_HANDLER(InsertNewLine, insert_new_line);
    PUT_DEFAULT_TIC_HANDLER(InsertBeforeBlock, insert_before_block);
    PUT_DEFAULT_TIC_HANDLER(InsertAfterBlock, insert_after_block);
    PUT_DEFAULT_TIC_HANDLER(Cancel, cancel);
    PUT_DEFAULT_TIC_HANDLER(Undo, undo);
    PUT_DEFAULT_TIC_HANDLER(Redo, redo);
    PUT_DEFAULT_TIC_HANDLER(Copy, copy);
    PUT_DEFAULT_TIC_HANDLER(Cut, cut);
    PUT_DEFAULT_TIC_HANDLER(Paste, paste);
    PUT_DEFAULT_TIC_HANDLER(ScrollUp, scroll_up);
    PUT_DEFAULT_TIC_HANDLER(ScrollDown, scroll_down);
    PUT_DEFAULT_TIC_HANDLER(MoveToPrevChar, move_to_prev_char);
    PUT_DEFAULT_TIC_HANDLER(MoveToNextChar, move_to_next_char);
    PUT_DEFAULT_TIC_HANDLER(MoveToPrevWord, move_to_prev_word);
    PUT_DEFAULT_TIC_HANDLER(MoveToNextWord, move_to_next_word);
    PUT_DEFAULT_TIC_HANDLER(MoveToPrevLine, move_to_prev_line);
    PUT_DEFAULT_TIC_HANDLER(MoveToNextLine, move_to_next_line);
    PUT_DEFAULT_TIC_HANDLER(MoveToStartOfLine, move_to_start_of_line);
    PUT_DEFAULT_TIC_HANDLER(MoveToEndOfLine, move_to_end_of_line);
    PUT_DEFAULT_TIC_HANDLER(MoveToStartOfBlock, move_to_start_of_block);
    PUT_DEFAULT_TIC_HANDLER(MoveToEndOfBlock, move_to_end_of_block);
    PUT_DEFAULT_TIC_HANDLER(MoveToStartOfDocument, move_to_start_of_document);
    PUT_DEFAULT_TIC_HANDLER(MoveToEndOfDocument, move_to_end_of_document);
    PUT_DEFAULT_TIC_HANDLER(MoveToPrevPage, move_to_prev_page);
    PUT_DEFAULT_TIC_HANDLER(MoveToNextPage, move_to_next_page);
    PUT_DEFAULT_TIC_HANDLER(MoveToPrevBlock, move_to_prev_block);
    PUT_DEFAULT_TIC_HANDLER(MoveToNextBlock, move_to_next_block);
    PUT_DEFAULT_TIC_HANDLER(DeletePrevChar, delete_prev_char);
    PUT_DEFAULT_TIC_HANDLER(DeleteNextChar, delete_next_char);
    PUT_DEFAULT_TIC_HANDLER(DeletePrevWord, delete_prev_word);
    PUT_DEFAULT_TIC_HANDLER(DeleteNextWord, delete_next_word);
    PUT_DEFAULT_TIC_HANDLER(DeleteToStartOfLine, delete_to_start_of_line);
    PUT_DEFAULT_TIC_HANDLER(DeleteToEndOfLine, delete_to_end_of_line);
    PUT_DEFAULT_TIC_HANDLER(SelectPrevChar, select_prev_char);
    PUT_DEFAULT_TIC_HANDLER(SelectNextChar, select_next_char);
    PUT_DEFAULT_TIC_HANDLER(SelectPrevWord, select_prev_word);
    PUT_DEFAULT_TIC_HANDLER(SelectNextWord, select_next_word);
    PUT_DEFAULT_TIC_HANDLER(SelectToPrevLine, select_to_prev_line);
    PUT_DEFAULT_TIC_HANDLER(SelectToNextLine, select_to_next_line);
    PUT_DEFAULT_TIC_HANDLER(SelectToStartOfLine, select_to_start_of_line);
    PUT_DEFAULT_TIC_HANDLER(SelectToEndOfLine, select_to_end_of_line);
    PUT_DEFAULT_TIC_HANDLER(SelectToStartOfBlock, select_to_start_of_block);
    PUT_DEFAULT_TIC_HANDLER(SelectToEndOfBlock, select_to_end_of_block);
    PUT_DEFAULT_TIC_HANDLER(SelectBlock, select_block);
    PUT_DEFAULT_TIC_HANDLER(SelectPrevPage, select_prev_page);
    PUT_DEFAULT_TIC_HANDLER(SelectNextPage, select_next_page);
    PUT_DEFAULT_TIC_HANDLER(SelectToStartOfDoc, select_to_start_of_doc);
    PUT_DEFAULT_TIC_HANDLER(SelectToEndOfDoc, select_to_end_of_doc);
    PUT_DEFAULT_TIC_HANDLER(SelectAll, select_all);

#undef PUT_DEFAULT_TIC_HANDLER
}

bool PrimaryEditor::focusNextPrevChild(bool next) {
    return false;
}

bool PrimaryEditor::event(QEvent *event) {
    if (!enabled()
        && (event->isInputEvent() || event->type() == QEvent::FocusIn
            || (event->isSinglePointEvent() && event->type() != QEvent::Wheel)
            || event->type() == QEvent::InputMethod)) {
        return true;
    }
    return QWidget::event(event);
}

void PrimaryEditor::resizeEvent(QResizeEvent *event) {
    prepare_canvas();
    update_text_view_geometry();
}

void PrimaryEditor::paintEvent(QPaintEvent *event) {
    QPainter p;

    auto  ctx  = context();
    auto &lock = ctx->lock;

    if (!enabled()) {
        canvas_.fill(palette().color(backgroundRole()));
        p.begin(&canvas_);
        draw(&p);
        p.end();
    } else if (ctx->cached_render_data_ready && lock.try_lock_read()) {
        canvas_.fill(palette().color(backgroundRole()));
        p.begin(&canvas_);
        p.setFont(text_font_);
        draw(&p);
        p.end();
        lock.unlock_read();
    }

    p.begin(this);
    p.drawPixmap(0, 0, canvas_);
    p.end();
}

void PrimaryEditor::focusInEvent(QFocusEvent *event) {
    if (readonly()) { return; }
    emit on_activate();
}

void PrimaryEditor::focusOutEvent(QFocusEvent *event) {
    Q_UNIMPLEMENTED();
}

void PrimaryEditor::keyPressEvent(QKeyEvent *event) {
    auto e = engine();
    if (!e->is_cursor_available() || e->preedit) { return; }

    auto &config = AppConfig::get_instance();
    auto &man    = config.primary_text_input_command_manager();

    man.push(e);
    const auto action = man.match(event);
    man.pop();

    QString inserted{};
    if (action == TextInputCommand::InsertPrintable) {
        inserted = TextInputCommandManager::translate_printable_char(event);
    }

    execute_text_input_command(action, inserted);
}

void PrimaryEditor::mousePressEvent(QMouseEvent *event) {
    const auto vpos = event->pos() - text_area().topLeft();
    switch (ma_state_) {
        case MouseActionState::Idle: {
            if (event->button() == Qt::MiddleButton) {
                ma_state_                  = MouseActionState::PreAutoScroll;
                auto_scroll_info_.base_pos = event->pos().y();
                auto_scroll_info_.ref_pos  = auto_scroll_info_.base_pos;
                save_and_set_cursor(Qt::SizeVerCursor);
            } else if (event->button() != Qt::LeftButton) {
                //! NOTE: nothing to do in the current version
            } else if (test_pos_in_sel_area(vpos)) {
                ma_state_ = MouseActionState::PreDragSelection;
            } else {
                ma_state_ = MouseActionState::PreSelectText;
                set_caret_loc_to_pos(vpos, false);
                ma_state_ = MouseActionState::SelectText;
            }
        } break;
        case MouseActionState::AutoScroll: {
            ma_state_                 = MouseActionState::Idle;
            auto_scroll_info_.enabled = false;
            restore_cursor();
        } break;
        default: {
            Q_UNREACHABLE();
        } break;
    }
}

void PrimaryEditor::mouseReleaseEvent(QMouseEvent *event) {
    const auto vpos = event->pos() - text_area().topLeft();
    switch (ma_state_) {
        case MouseActionState::Idle: {
        } break;
        case MouseActionState::PreAutoScroll: {
            if (event->button() == Qt::MiddleButton) {
                ma_state_                 = MouseActionState::AutoScroll;
                auto_scroll_info_.enabled = true;
            } else {
                Q_UNREACHABLE();
            }
        } break;
        case MouseActionState::LightAutoScroll: {
            if (event->button() == Qt::MiddleButton) {
                ma_state_                 = MouseActionState::Idle;
                auto_scroll_info_.enabled = false;
                restore_cursor();
            } else {
                Q_UNREACHABLE();
            }
        } break;
        case MouseActionState::PreDragSelection: {
            if (event->button() == Qt::LeftButton) {
                ma_state_ = MouseActionState::SetCaret;
                set_caret_loc_to_pos(vpos, false);
                ma_state_ = MouseActionState::Idle;
            } else {
                Q_UNREACHABLE();
            }
        } break;
        case MouseActionState::SelectText: {
            if (event->button() == Qt::LeftButton) {
                ma_state_ = MouseActionState::Idle;
            } else {
                Q_UNREACHABLE();
            }
        } break;
        case MouseActionState::DragSelection: {
            if (event->button() == Qt::LeftButton) {
                ma_state_ = MouseActionState::DragSelectionDone;
                if (!readonly()) { complete_text_drag(); }
                ma_state_ = MouseActionState::Idle;
            } else {
                Q_UNREACHABLE();
            }
        } break;
        default: {
            Q_UNREACHABLE();
        } break;
    }
}

void PrimaryEditor::mouseDoubleClickEvent(QMouseEvent *event) {
    reset_mouse_action_state();

    if (event->button() == Qt::LeftButton) {
        const auto vpos = event->pos() - text_area().topLeft();
        const auto e    = engine();
        if (e->preedit) { return; }
        const auto loc = context()->get_textloc_at_rel_vpos(vpos, true);
        Q_ASSERT(loc.block_index != -1);
        const auto block = e->active_blocks[loc.block_index];
        select(block->text_pos, block->text_pos + block->text_len());
    }
}

void PrimaryEditor::mouseMoveEvent(QMouseEvent *event) {
    const auto vpos = event->pos() - text_area().topLeft();
    switch (ma_state_) {
        case MouseActionState::Idle: {
        } break;
        case MouseActionState::PreAutoScroll: {
            if (event->buttons() & Qt::MiddleButton) {
                ma_state_                 = MouseActionState::LightAutoScroll;
                auto_scroll_info_.enabled = true;
                auto_scroll_info_.ref_pos = event->pos().y();
            } else {
                Q_UNREACHABLE();
            }
        } break;
        case MouseActionState::LightAutoScroll: {
            if (event->buttons() & Qt::MiddleButton) {
                ma_state_                 = MouseActionState::LightAutoScroll;
                auto_scroll_info_.ref_pos = event->pos().y();
            } else {
                Q_UNREACHABLE();
            }
        } break;
        case MouseActionState::AutoScroll: {
            if (event->buttons() == Qt::NoButton) {
                ma_state_                 = MouseActionState::AutoScroll;
                auto_scroll_info_.ref_pos = event->pos().y();
            } else {
                Q_UNREACHABLE();
            }
        } break;
        case MouseActionState::SelectText: {
            if (event->buttons() & Qt::LeftButton) {
                ma_state_ = MouseActionState::SelectText;
                //! FIXME: handle out-of-bound selection
                set_caret_loc_to_pos(vpos, true);
            } else {
                Q_UNREACHABLE();
            }
        } break;
        case MouseActionState::PreDragSelection: {
            if (event->buttons() & Qt::LeftButton) {
                ma_state_                  = MouseActionState::DragSelection;
                text_drag_state_.enabled   = true;
                text_drag_state_.caret_loc = context()->get_textloc_at_rel_vpos(vpos, true);
            } else {
                Q_UNREACHABLE();
            }
        } break;
        case MouseActionState::DragSelection: {
            if (event->buttons() & Qt::LeftButton) {
                ma_state_ = MouseActionState::DragSelection;
                //! FIXME: handle out-of-bound drag
                text_drag_state_.caret_loc = context()->get_textloc_at_rel_vpos(vpos, true);
            } else {
                Q_UNREACHABLE();
            }
        } break;
        default: {
            Q_UNREACHABLE();
        } break;
    }
}

void PrimaryEditor::wheelEvent(QWheelEvent *event) {
    //! NOTE: exclusive scroll control
    if (auto_scroll_info_.enabled) { return; }

    if (engine()->is_empty()) { return; }

    const double ratio        = 1.0 / 180 * 3;
    const double line_spacing = this->line_spacing();
    const double delta        = -event->angleDelta().y() * line_spacing * ratio;
    request_scroll((event->modifiers() & Qt::ControlModifier) ? delta * 8 : delta);
}

void PrimaryEditor::dragEnterEvent(QDragEnterEvent *event) {
    Q_UNIMPLEMENTED();
}

void PrimaryEditor::dropEvent(QDropEvent *event) {
    Q_UNIMPLEMENTED();
}

void PrimaryEditor::inputMethodEvent(QInputMethodEvent *event) {
    if (readonly()) { return; }

    const auto ctx = context();
    const auto e   = engine();
    if (!e->is_cursor_available()) { return; }

    reset_mouse_action_state();

    if (const auto preedit_text = event->preeditString(); !preedit_text.isEmpty()) {
        if (!e->preedit) { ctx->begin_preedit(); }
        ctx->update_preedit(event->preeditString());
    } else {
        insert(event->commitString(), false);
    }

    request_update(false);
}

QVariant PrimaryEditor::inputMethodQuery(Qt::InputMethodQuery query) const {
    switch (query) {
        case Qt::ImCursorRectangle: {
            const auto &e = engine();
            if (e->is_dirty() || !e->is_cursor_available()) { return QRect(0, 0, 1, 1); }

            const auto text_area = this->text_area();
            auto [x_pos, y_pos]  = context_->get_vpos_at_cursor();
            const auto  pos = QPoint(x_pos, y_pos - context_->viewport_y_pos) + text_area.topLeft();
            const QSize size(1, e->line_height);
            QRect       bb(pos, size);

            const int threshould = 64;
            const int offset_to_screen_bottom =
                screen()->size().height() - mapToGlobal(QPoint(0, bb.bottom())).y();
            if (offset_to_screen_bottom < threshould) {
                const int preferred_ime_editor_height = 128;
                bb.translate(0, -preferred_ime_editor_height);
                bb.setHeight(1);
            }

            return bb;
        } break;
        default: {
            return QWidget::inputMethodQuery(query);
        } break;
    }
}

} // namespace jwrite::ui
