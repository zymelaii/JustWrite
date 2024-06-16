#include <jwrite/VisualTextEditContext.h>
#include <jwrite/ProfileUtils.h>

namespace jwrite {

VisualTextEditContext::VisualTextEditContext(const QFontMetrics &fm, int width)
    : engine(fm, width)
    , edit_cursor_pos{0}
    , cursor_moved{false}
    , vertical_move_state{false}
    , viewport_width{engine.max_width}
    , viewport_height{0}
    , cached_render_data_ready{false} {
    engine.set_text_ref_unsafe(&edit_text, 0);
    sel.clear();
}

void VisualTextEditContext::resize_viewport(int width, int height) {
    Q_ASSERT(width >= 0);
    Q_ASSERT(height >= 0);

    lock.lock_write();

    //! FIXME: viewport_y_pos will be dirty after viewport width changed

    if (height != viewport_height) {
        if (height > viewport_height) { cached_render_data_ready = false; }
        viewport_height = height;
    }

    if (width != viewport_width) {
        engine.reset_max_width(width);
        cached_render_data_ready = false;
    }

    lock.unlock_write();
}

void VisualTextEditContext::prepare_render_data() {
    cached_render_data_ready = cached_render_data_ready && !engine.is_dirty() && !cursor_moved;
    if (cached_render_data_ready) { return; }

    lock.lock_write();

    jwrite_profiler_start(TextEngineRenderCost);
    engine.render();
    jwrite_profiler_record(TextEngineRenderCost);

    const auto  &fm                 = engine.fm;
    const double line_spacing       = engine.line_height * engine.line_spacing_ratio;
    const int    viewport_width     = engine.max_width;
    const double max_viewport_y_pos = viewport_y_pos + viewport_height;

    auto &d                = cached_render_state;
    d.found_visible_block  = false;
    d.active_block_visible = false;
    d.visible_block        = {-1, -1};
    d.visible_sel          = {-1, -1};
    d.cached_block_y_pos.clear();

    double y_pos = 0.0;

    const int total_blocks = engine.active_blocks.size();
    for (int index = 0; index < total_blocks; ++index) {
        const auto   block  = engine.active_blocks[index];
        const double stride = block->lines.size() * line_spacing + engine.block_spacing;
        if (y_pos + stride - engine.block_spacing < viewport_y_pos) {
            y_pos += stride;
            continue;
        } else if (y_pos > max_viewport_y_pos) {
            break;
        } else if (!d.found_visible_block) {
            d.first_visible_block_y_pos = y_pos;
            d.found_visible_block       = true;
        }
        d.cached_block_y_pos[index] = y_pos;
        if (d.visible_block.first == -1) { d.visible_block.first = index; }
        d.visible_block.last = index;
        if (index == engine.active_block_index) {
            //! FIXME: maybe there is another design about y-pos-range in the future
            d.active_block_y_start = y_pos;
            d.active_block_y_end   = y_pos + stride - engine.block_spacing;
            d.active_block_visible = true;
        }
        y_pos += stride;
    }

    if (has_sel()) {
        jwrite_profiler_start(SelectionLocatingCost);
        const int hint      = sel.from < sel.to ? -1 : 1;
        d.sel_loc_from      = get_textloc_at_pos(qMin(sel.from, sel.to), hint);
        d.sel_loc_to        = get_textloc_at_pos(qMax(sel.from, sel.to), -hint);
        d.visible_sel.first = qMax(d.sel_loc_from.block_index, d.visible_block.first);
        d.visible_sel.last  = qMin(d.sel_loc_to.block_index, d.visible_block.last);
        if (d.visible_sel.first > d.visible_sel.last) {
            d.visible_sel.first = -1;
            d.visible_sel.last  = -1;
        }
        jwrite_profiler_record(SelectionLocatingCost);
    }

    cached_render_data_ready = true;

    lock.unlock_write();
}

VisualTextEditContext::TextLoc VisualTextEditContext::current_textloc() const {
    return {
        .block_index = engine.active_block_index,
        .row         = engine.cursor.row,
        .col         = engine.cursor.col,
        .pos         = engine.cursor.pos,
    };
}

VisualTextEditContext::TextLoc VisualTextEditContext::get_textloc_at_pos(int pos, int hint) const {
    TextLoc   loc{.block_index = -1};
    const int total_blocks = engine.active_blocks.size();
    for (int i = 0; i < total_blocks; ++i) {
        const auto block = engine.active_blocks[i];
        if (!(pos >= block->text_pos && pos <= block->text_pos + block->text_len())) { continue; }
        const int relpos = pos - block->text_pos;
        for (int j = 0; j < block->lines.size(); ++j) {
            const auto line = block->lines[j];
            const int  col  = relpos - line.text_offset();
            if (col > line.text_len()) { continue; }
            if (hint < 0 && i + 1 < total_blocks && relpos == block->text_len()) {
                loc.block_index = i + 1;
                loc.pos         = 0;
                loc.row         = 0;
                loc.col         = 0;
            } else {
                loc.block_index = i;
                loc.pos         = relpos;
                loc.row         = j;
                loc.col         = col;
            }
            return loc;
        }
    }
    return loc;
}

bool VisualTextEditContext::set_cursor_to_textloc(const TextLoc &loc, int hint) {
    Q_ASSERT(!engine.preedit);
    if (!(loc.block_index >= 0 && loc.block_index < engine.active_blocks.size())) { return false; }
    const auto block = engine.active_blocks[loc.block_index];
    if (hint == 0) {
        if (!(loc.row >= 0 && loc.row < block->lines.size())) { return false; }
        const auto line = block->lines[loc.row];
        if (!(loc.col >= 0 && loc.col <= line.text_len())) { return false; }
        engine.cursor.row = loc.row;
        engine.cursor.col = loc.col;
        engine.cursor.pos = line.text_offset() + loc.col;
    } else if (!(loc.pos >= 0 && loc.pos <= block->text_len())) {
        return false;
    }
    engine.active_block_index = loc.block_index;
    if (hint != 0) {
        engine.cursor.pos = loc.pos;
        engine.sync_cursor_row_col(hint);
    }
    edit_cursor_pos     = block->text_pos + engine.cursor.pos;
    vertical_move_state = false;
    cursor_moved        = true;
    return true;
}

int VisualTextEditContext::get_column_at_vpos(const TextLine &line, double x_pos) const {
    if (line.parent->is_dirty()) { line.parent->render(); }

    const auto  &fm      = engine.fm;
    const auto  &text    = line.text();
    const int    len     = text.length();
    const double spacing = line.char_spacing();

    double x   = line.is_first_line() ? engine.standard_char_width * 2 : 0;
    int    col = 0;
    while (col < len) {
        const double advance    = fm.horizontalAdvance(text[col]);
        const double char_width = advance + spacing;
        if (x + char_width * 0.5 > x_pos) { break; }
        x += char_width;
        ++col;
    }

    return col;
}

int VisualTextEditContext::get_vpos_at_cursor_col() const {
    Q_ASSERT(!engine.is_dirty());
    Q_ASSERT(engine.is_cursor_available());
    const auto &fm            = engine.fm;
    const auto &cursor        = engine.cursor;
    const auto &line          = engine.current_line();
    const int   leading_space = line.is_first_line() ? engine.standard_char_width * 2 : 0;
    double      x_pos         = leading_space + cursor.col * line.char_spacing();
    for (const auto c : line.text().left(cursor.col)) { x_pos += fm.horizontalAdvance(c); }
    return x_pos;
}

VisualTextEditContext::TextLoc
    VisualTextEditContext::get_textloc_at_rel_vpos(const QPoint &pos, bool clip) const {
    Q_ASSERT(!engine.preedit);

    TextLoc loc{};

    const double line_spacing = engine.line_height * engine.line_spacing_ratio;
    const double y_pos        = clip ? qBound(0, pos.y(), viewport_height) : pos.y();
    const double target_y_pos = y_pos + viewport_y_pos;

    int    index     = -1;
    double rel_y_pos = 0.0;

    const auto &d               = cached_render_state;
    const bool  use_cached_data = cached_render_data_ready && d.found_visible_block && pos.y() >= 0
                              && pos.y() <= viewport_height;

    if (use_cached_data) {
        index = d.visible_block.first;
        while (index < d.visible_block.last) {
            const auto block = engine.active_blocks[index];
            Q_ASSERT(!block->is_dirty());
            Q_ASSERT(d.cached_block_y_pos.contains(index));
            const double block_y_pos   = d.cached_block_y_pos[index];
            const double stride        = block->lines.size() * line_spacing + engine.block_spacing;
            const double block_y_limit = block_y_pos + stride - engine.block_spacing * 0.5;
            if (target_y_pos <= block_y_limit) { break; }
            ++index;
        }
        rel_y_pos = target_y_pos - d.cached_block_y_pos[index];
    } else {
        double y_pos = 0.0;
        index        = 0;
        while (index + 1 < engine.active_blocks.size()) {
            const auto   block         = engine.active_blocks[index];
            const double stride        = block->lines.size() * line_spacing + engine.block_spacing;
            const double block_y_limit = y_pos + stride - engine.block_spacing * 0.5;
            if (target_y_pos <= block_y_limit) { break; }
            y_pos += stride;
            ++index;
        }
        rel_y_pos = target_y_pos - y_pos;
    }

    const int  row_stride = qMax(0, static_cast<int>(rel_y_pos / line_spacing));
    const auto block      = engine.active_blocks[index];
    const int  last_line  = block->lines.size() - 1;
    const int  row        = qBound(0, row_stride, last_line);
    const auto line       = block->lines[row];
    const int  col        = get_column_at_vpos(line, pos.x());

    loc.block_index = index;
    loc.row         = row;
    loc.col         = col;
    loc.pos         = line.text_offset() + col;

    return loc;
}

QPointF VisualTextEditContext::get_vpos_at_cursor() const {
    Q_ASSERT(!engine.is_dirty());
    Q_ASSERT(engine.is_cursor_available());

    const auto &fm     = engine.fm;
    const auto &cursor = engine.cursor;
    const auto &block  = engine.current_block();
    const auto &line   = block->current_line();

    const auto &d               = cached_render_state;
    const bool  use_cached_data = cached_render_data_ready && d.found_visible_block
                              && d.visible_block.first <= engine.active_block_index
                              && d.visible_block.last >= engine.active_block_index;

    const int    leading_space = line.is_first_line() ? engine.standard_char_width * 2 : 0;
    const double line_spacing  = engine.line_height * engine.line_spacing_ratio;

    double x_pos = leading_space + cursor.col * line.char_spacing();
    for (const auto c : line.text().left(cursor.col)) { x_pos += fm.horizontalAdvance(c); }

    double y_pos = 0.0;
    if (use_cached_data) {
        y_pos = d.cached_block_y_pos[engine.active_block_index];
    } else {
        int start = -1;
        int end   = -1;
        int inc   = 0;

        if (!d.found_visible_block) {
            y_pos = 0.0;
            start = 0;
            end   = engine.active_block_index;
            inc   = 1;
        } else if (d.visible_block.first > engine.active_block_index) {
            y_pos = d.cached_block_y_pos[d.visible_block.first];
            start = d.visible_block.first - 1;
            end   = engine.active_block_index - 1;
            inc   = -1;
        } else if (d.visible_block.last < engine.active_block_index) {
            y_pos = d.cached_block_y_pos[d.visible_block.last];
            start = d.visible_block.last;
            end   = engine.active_block_index;
            inc   = 1;
        } else {
            //! NOTE: cached-render-data is not ready, but the cursor is in the visible block, only
            //! the y-pos is dirty
            y_pos = 0.0;
            start = 0;
            end   = engine.active_block_index;
            inc   = 1;
        }

        double offset = 0.0;
        for (int i = start; i != end; i += inc) {
            const auto   block   = engine.active_blocks[i];
            const double stride  = block->lines.size() * line_spacing + engine.block_spacing;
            offset              += stride;
        }

        y_pos += inc > 0 ? offset : -offset;
    }

    y_pos += cursor.row * line_spacing;

    return QPointF(x_pos, y_pos);
}

void VisualTextEditContext::begin_preedit() {
    Q_ASSERT(!engine.preedit);
    remove_sel_region(nullptr);
    engine.begin_preedit(preedit_text);
}

void VisualTextEditContext::update_preedit(const QString &text) {
    Q_ASSERT(engine.is_cursor_available());
    Q_ASSERT(engine.preedit);
    const auto &cursor                = engine.cursor;
    const auto &saved_cursor          = engine.saved_cursor;
    const int   last_preedit_text_len = cursor.pos - saved_cursor.pos;
    preedit_text.remove(saved_cursor.pos, last_preedit_text_len);
    preedit_text.insert(saved_cursor.pos, text);
    engine.update_preedit_text(text.length());
    cursor_moved = true;
}

void VisualTextEditContext::quit_preedit() {
    if (!engine.preedit) { return; }
    const auto &cursor                = engine.cursor;
    const auto &saved_cursor          = engine.saved_cursor;
    const auto  last_preedit_text_len = cursor.pos - saved_cursor.pos;
    preedit_text.remove(saved_cursor.pos, last_preedit_text_len);
    engine.update_preedit_text(0);
    engine.commit_preedit();
}

void VisualTextEditContext::commit_preedit() {
    Q_ASSERT(engine.is_cursor_available());
    Q_ASSERT(engine.preedit);
    engine.commit_preedit();
}

void VisualTextEditContext::remove_sel_region(QString *deleted_text) {
    Q_ASSERT(!engine.preedit);
    if (!has_sel()) { return; }
    const int len = sel.len();
    move_within_sel_region(-1);
    del(len, true, deleted_text);
}

int VisualTextEditContext::move_within_sel_region(int hint) {
    Q_ASSERT(!engine.preedit);
    if (!has_sel() || hint == 0) { return hint; }
    const int  pos = hint > 0 ? qMax(sel.from, sel.to) : qMin(sel.from, sel.to);
    const auto loc = get_textloc_at_pos(pos, -1);
    set_cursor_to_textloc(loc, hint);
    unset_sel();
    cursor_moved = true;
    return hint > 0 ? hint - 1 : hint + 1;
}

void VisualTextEditContext::move(int offset, bool extend_sel) {
    Q_ASSERT(engine.is_cursor_available());

    if (offset == 0) { return; }

    //! ATTENTION: restore the state in vertical_move if needed
    vertical_move_state = false;

    if (has_sel() && !extend_sel) { offset = move_within_sel_region(offset); }

    //! FIXME: take within-sel-move into account?
    bool      edit_cursor_moved  = false;
    const int edit_cursor_offset = engine.commit_movement(offset, &edit_cursor_moved, false);

    if (extend_sel) {
        if (!has_sel()) { sel.from = edit_cursor_pos; }
        sel.to = edit_cursor_pos + edit_cursor_offset;
    }

    edit_cursor_pos += edit_cursor_offset;

    cursor_moved = true;
}

void VisualTextEditContext::move_to(int pos, bool extend_sel) {
    Q_ASSERT(engine.is_cursor_available());

    //! ATTENTION: restore the state in vertical_move if needed
    vertical_move_state = false;

    if (!has_sel() && extend_sel) { sel.from = edit_cursor_pos; }

    const int offset  = pos - edit_cursor_pos;
    edit_cursor_pos  += engine.commit_movement(offset, nullptr, true);

    if (extend_sel) {
        sel.to = edit_cursor_pos;
    } else if (has_sel()) {
        unset_sel();
    }

    cursor_moved = true;
}

void VisualTextEditContext::del(int times, bool hard_mode, QString *deleted_text) {
    Q_ASSERT(engine.is_cursor_available());
    Q_ASSERT(!has_sel());
    Q_ASSERT(!engine.preedit);

    //! TODO: better solution
    if (deleted_text) {
        const bool saved_cursor_moved    = cursor_moved;
        const int  saved_edit_cursor_pos = edit_cursor_pos;
        const auto saved_loc             = current_textloc();
        engine.commit_movement(times, nullptr, hard_mode);
        const auto dest_loc = current_textloc();

        const auto &loc_start = times > 0 ? saved_loc : dest_loc;
        const auto &loc_end   = times > 0 ? dest_loc : saved_loc;

        const bool succeed = set_cursor_to_textloc(saved_loc, 0);
        Q_ASSERT(succeed);
        cursor_moved = saved_cursor_moved;

        if (loc_start.block_index == loc_end.block_index) {
            const int pos = engine.active_blocks[loc_start.block_index]->text_pos + loc_start.pos;
            *deleted_text = edit_text.mid(pos, loc_end.pos - loc_start.pos);
        } else {
            QStringList blocks{};
            blocks << engine.active_blocks[loc_start.block_index]
                          ->text()
                          .mid(loc_start.pos)
                          .toString();
            for (int i = loc_start.block_index + 1; i < loc_end.block_index; ++i) {
                blocks << engine.active_blocks[i]->text().toString();
            }
            blocks
                << engine.active_blocks[loc_end.block_index]->text().left(loc_end.pos).toString();
            *deleted_text = blocks.join('\n');
        }

        Q_ASSERT(saved_edit_cursor_pos == edit_cursor_pos);
    }

    int       deleted             = 0;
    const int edit_cursor_offset  = engine.commit_deletion(times, deleted, hard_mode);
    edit_cursor_pos              += edit_cursor_offset;

    edit_text.remove(edit_cursor_pos, deleted);

    cursor_moved = true;
}

void VisualTextEditContext::insert(const QString &text) {
    Q_ASSERT(engine.is_cursor_available());

    if (has_sel()) { remove_sel_region(nullptr); }

    const int len = text.length();

    engine.commit_insertion(len);
    edit_text.insert(edit_cursor_pos, text);
    edit_cursor_pos += len;

    cursor_moved = true;
}

bool VisualTextEditContext::vertical_move(bool up) {
    if (!engine.is_cursor_available()) { return false; }

    const int move_hint = up ? -1 : 1;
    if (has_sel()) { move_within_sel_region(move_hint); }

    const auto &fm          = engine.fm;
    const auto &blocks      = engine.active_blocks;
    const auto &block       = engine.current_block();
    const auto &line        = block->current_line();
    const auto &cursor      = engine.cursor;
    const int   block_index = engine.active_block_index;

    if (!vertical_move_state) {
        const int leading_space = line.is_first_line() ? engine.standard_char_width * 2 : 0;
        vertical_move_ref_pos   = leading_space + cursor.col * line.char_spacing();
        for (const auto c : line.text().left(cursor.col)) {
            vertical_move_ref_pos += fm.horizontalAdvance(c);
        }
        vertical_move_state = true;
    }

    TextLine *target_line = nullptr;
    bool      cross_block = false;

    do {
        if (up) {
            if (cursor.row > 0) {
                target_line = &block->lines[cursor.row - 1];
                break;
            }
            if (block_index == 0) {
                move_to(block->text_pos, false);
                break;
            }
            target_line = &blocks[block_index - 1]->lines.back();
            cross_block = true;
        } else {
            if (cursor.row + 1 < block->lines.size()) {
                target_line = &block->lines[cursor.row + 1];
                break;
            }
            if (block_index + 1 == blocks.size()) {
                move_to(block->text_pos + block->text_len(), false);
                break;
            }
            target_line = &blocks[block_index + 1]->lines.front();
            cross_block = true;
        }
    } while (0);

    if (!target_line) {
        vertical_move_state = false;
        return false;
    }

    const auto &target_text  = target_line->text();
    int         target_col   = 0;
    double      target_x_pos = target_line->is_first_line() ? engine.standard_char_width * 2 : 0;

    while (target_col < target_text.length()) {
        const double advance    = fm.horizontalAdvance(target_text[target_col]);
        const double char_width = advance + target_line->char_spacing();
        if (vertical_move_ref_pos < target_x_pos + 0.5 * char_width) { break; }
        target_x_pos += char_width;
        ++target_col;
    }

    const int target_pos = target_line->text_offset() + target_line->parent->text_pos + target_col;
    int       offset     = target_pos - edit_cursor_pos;
    if (cross_block) { offset += move_hint; }

    move(offset, false);

    //! NOTE: default hint of move action would block the movement to the bound of line, mannually
    //! shift it again to ensure the cursor could arrive at the target line
    if (!cross_block && cursor.row != target_line->line_nr) {
        Q_ASSERT(qAbs(target_line->line_nr - cursor.row) == 1);
        const int diff = target_line->line_nr - cursor.row;
        move(diff, false);
        move(-diff, false);
    }

    //! move always set vertical_move to false, restore the value here
    vertical_move_state = true;

    cursor_moved = true;

    return true;
}

void VisualTextEditContext::scroll_to(double pos) {
    viewport_y_pos           = pos;
    cached_render_data_ready = false;
}

QDebug operator<<(QDebug stream, const VisualTextEditContext::TextLoc &text_loc) {
    QDebugStateSaver saver(stream);
    stream.nospace() << "TextLoc[index=" << text_loc.block_index << ",row=" << text_loc.row
                     << ",col=" << text_loc.col << ",pos=" << text_loc.pos << "]";
    return stream;
}

} // namespace jwrite
