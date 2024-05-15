#include "VisualTextEditContext.h"
#include "ProfileUtils.h"

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

    //! FIXME: viewport_y_pos will be dirty after viewport width changed

    if (height != viewport_height) {
        if (height < viewport_height) { cached_render_data_ready = false; }
        viewport_height = height;
    }

    if (width != viewport_width) {
        engine.reset_max_width(width);
        cached_render_data_ready = false;
    }
}

void VisualTextEditContext::prepare_render_data() {
    cached_render_data_ready = cached_render_data_ready && !engine.is_dirty() && !cursor_moved;
    if (cached_render_data_ready) { return; }

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
        if (y_pos + stride < viewport_y_pos) {
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
        d.sel_loc_from      = get_textloc_at_pos(qMin(sel.from, sel.to));
        d.sel_loc_to        = get_textloc_at_pos(qMax(sel.from, sel.to));
        d.visible_sel.first = qMax(d.sel_loc_from.block_index, d.visible_block.first);
        d.visible_sel.last  = qMin(d.sel_loc_to.block_index, d.visible_block.last);
        if (d.visible_sel.first > d.visible_sel.last) {
            d.visible_sel.first = -1;
            d.visible_sel.last  = -1;
        }
        jwrite_profiler_record(SelectionLocatingCost);
    }

    cached_render_data_ready = true;
}

VisualTextEditContext::TextLoc VisualTextEditContext::current_textloc() const {
    return {
        .block_index = engine.active_block_index,
        .row         = engine.cursor.row,
        .col         = engine.cursor.col,
        .pos         = edit_cursor_pos,
    };
}

VisualTextEditContext::TextLoc VisualTextEditContext::get_textloc_at_pos(int pos) const {
    TextLoc loc{.block_index = -1};
    for (int i = 0; i < engine.active_blocks.size(); ++i) {
        const auto block = engine.active_blocks[i];
        if (!(pos >= block->text_pos && pos <= block->text_pos + block->text_len())) { continue; }
        const int relpos = pos - block->text_pos;
        for (int j = 0; j < block->lines.size(); ++j) {
            const auto line = block->lines[j];
            const int  col  = relpos - line.text_offset();
            if (col > line.text_len()) { continue; }
            loc.block_index = i;
            loc.pos         = relpos;
            loc.row         = j;
            loc.col         = col;
            return loc;
        }
    }
    return loc;
}

bool VisualTextEditContext::set_cursor_to_textloc(const TextLoc &loc, int hint) {
    Q_ASSERT(!engine.preedit);
    if (!(loc.block_index >= 0 && loc.block_index < engine.active_blocks.size())) { return false; }
    const auto block = engine.active_blocks[loc.block_index];
    const int  pos   = loc.pos - block->text_pos;
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
    Q_ASSERT(!line.parent->is_dirty());

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

VisualTextEditContext::TextLoc VisualTextEditContext::get_textloc_at_vpos(const QPoint &pos) const {
    Q_ASSERT(!engine.preedit);

    TextLoc loc{};

    const double line_spacing = engine.line_height * engine.line_spacing_ratio;
    const double target_y_pos = pos.y() + viewport_y_pos;

    int    index     = -1;
    double rel_y_pos = 0.0;

    if (cached_render_data_ready) {
        const auto &d = cached_render_state;
        if (!d.found_visible_block) { return loc; }
        if (!(pos.y() >= 0 && pos.y() <= viewport_height)) { return loc; }
        index = d.visible_block.first;
        while (index < d.visible_block.last) {
            const auto block = engine.active_blocks[index];
            Q_ASSERT(d.cached_block_y_pos.contains(index));
            const double block_y_pos   = d.cached_block_y_pos[index];
            const double stride        = block->lines.size() * line_spacing + engine.block_spacing;
            const double block_y_limit = block_y_pos + stride;
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
            const double block_y_limit = y_pos + stride;
            if (target_y_pos <= block_y_limit) { break; }
            y_pos += stride;
            ++index;
        }
        rel_y_pos = target_y_pos - y_pos;
    }

    const auto block     = engine.active_blocks[index];
    const int  last_line = block->lines.size() - 1;
    const int  row       = qBound(0, static_cast<int>(rel_y_pos / line_spacing), last_line);
    const auto line      = block->lines[row];
    const int  col       = get_column_at_vpos(line, pos.x());

    loc.block_index = index;
    loc.row         = row;
    loc.col         = col;
    loc.pos         = line.text_offset() + col;

    return loc;
}

void VisualTextEditContext::begin_preedit() {
    Q_ASSERT(!engine.preedit);
    remove_sel_region();
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

void VisualTextEditContext::remove_sel_region() {
    Q_ASSERT(!engine.preedit);
    if (!has_sel()) { return; }
    const int len = sel.len();
    move_within_sel_region(-1);
    del(len, true);
}

int VisualTextEditContext::move_within_sel_region(int hint) {
    Q_ASSERT(!engine.preedit);
    if (!has_sel() || hint == 0) { return hint; }
    const int  pos = hint > 0 ? qMax(sel.from, sel.to) : qMin(sel.from, sel.to);
    const auto loc = get_textloc_at_pos(pos);
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

void VisualTextEditContext::del(int times, bool hard_mode) {
    Q_ASSERT(engine.is_cursor_available());

    if (has_sel()) {
        remove_sel_region();
    } else {
        int       deleted             = 0;
        const int edit_cursor_offset  = engine.commit_deletion(times, deleted, hard_mode);
        edit_cursor_pos              += edit_cursor_offset;
        edit_text.remove(edit_cursor_pos, deleted);
    }

    cursor_moved = true;
}

void VisualTextEditContext::insert(const QString &text) {
    Q_ASSERT(engine.is_cursor_available());

    if (has_sel()) { remove_sel_region(); }

    const int len = text.length();

    engine.commit_insertion(len);
    edit_text.insert(edit_cursor_pos, text);
    edit_cursor_pos += len;

    cursor_moved = true;
}

bool VisualTextEditContext::vertical_move(bool up) {
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

    //! move always set vertical_move to false, restore the value here
    vertical_move_state = true;

    cursor_moved = true;

    return true;
}

void VisualTextEditContext::scroll_to(double pos) {
    viewport_y_pos           = pos;
    cached_render_data_ready = false;
}

} // namespace jwrite
