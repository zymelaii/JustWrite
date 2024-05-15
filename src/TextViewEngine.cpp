#include "TextViewEngine.h"

namespace jwrite {

void TextLine::mark_as_dirty() const {
    parent->mark_as_dirty(line_nr);
}

QStringView TextLine::text() const {
    return parent->text_of_line(line_nr);
}

int TextLine::text_len() const {
    return text().length();
}

int TextLine::text_offset() const {
    return parent->offset_of_line(line_nr);
}

double TextLine::char_spacing() const {
    const int len = endp_offset - text_offset();
    return len < 2 ? 0.0 : cached_mean_width * 1.0 / (len - 1);
}

bool TextLine::is_first_line() const {
    return line_nr == 0;
}

void TextBlock::reset(const QString *ref, int pos) {
    text_ref = ref;
    text_pos = pos;
    lines.clear();
    dirty_line_nr = -1;

    TextLine line;
    line.parent      = this;
    line.line_nr     = 0;
    line.endp_offset = 0;
    lines.push_back(line);

    mark_as_dirty(0);
}

void TextBlock::release() {
    //! ATTENTION: callee must remove block from active_blocks
    parent->block_pool.append(this);
}

void TextBlock::mark_as_dirty(int line_nr) {
    Q_ASSERT(line_nr >= 0 && line_nr < lines.size());
    dirty_line_nr = !is_dirty() ? line_nr : qMin(dirty_line_nr, line_nr);
    parent->mark_as_dirty();
}

bool TextBlock::is_dirty() const {
    Q_ASSERT(dirty_line_nr == -1 || dirty_line_nr >= 0);
    return dirty_line_nr != -1;
}

void TextBlock::join_dirty_lines() {
    Q_ASSERT(is_dirty());
    lines[dirty_line_nr].endp_offset = lines.back().endp_offset;
    lines.remove(dirty_line_nr + 1, lines.size() - dirty_line_nr - 1);
}

int TextBlock::text_len() const {
    return lines.back().endp_offset;
}

int TextBlock::offset_of_line(int index) const {
    Q_ASSERT(index >= 0 && index < lines.size());
    return index == 0 ? 0 : lines[index - 1].endp_offset;
}

int TextBlock::len_of_line(int index) const {
    Q_ASSERT(index >= 0 && index < lines.size());
    return lines[index].endp_offset - offset_of_line(index);
}

TextLine &TextBlock::current_line() {
    Q_ASSERT(
        parent->active_block_index != -1
        && this == parent->active_blocks[parent->active_block_index]);
    return lines[parent->cursor.row];
}

const TextLine &TextBlock::current_line() const {
    Q_ASSERT(
        parent->active_block_index != -1
        && this == parent->active_blocks[parent->active_block_index]);
    return lines[parent->cursor.row];
}

QStringView TextBlock::text_of_line(int index) const {
    Q_ASSERT(index >= 0 && index < lines.size());
    Q_ASSERT(text_ref);
    const int offset = offset_of_line(index);
    return QStringView(*text_ref).mid(text_pos + offset, lines[index].endp_offset - offset);
}

QStringView TextBlock::text() const {
    Q_ASSERT(text_ref);
    return QStringView(*text_ref).mid(text_pos, text_len());
}

void TextBlock::squeeze_and_extend_last_line(int length) {
    Q_ASSERT(!lines.isEmpty());
    Q_ASSERT(length >= 0 && length < len_of_line(lines.size() - 1));
    TextLine line;
    line.parent               = this;
    line.line_nr              = lines.size();
    line.endp_offset          = lines.back().endp_offset;
    lines.back().endp_offset -= length;
    lines.push_back(line);
}

void TextBlock::render() {
    Q_ASSERT(is_dirty());
    Q_ASSERT(!lines.isEmpty());
    join_dirty_lines();
    Q_ASSERT(dirty_line_nr + 1 == lines.size());
    const int leading_space_width = parent->standard_char_width * 2;
    while (true) {
        auto      &line      = lines.back();
        const auto text      = line.text();
        const int  max_width = parent->max_width - (line.is_first_line() ? leading_space_width : 0);
        int        text_width = max_width;
        const int  text_len   = TextViewEngine::get_bounding_text_len(parent->fm, text, text_width);
        const int  text_len_diff = text.length() - text_len;
        line.cached_text_width   = text_width;
        line.cached_mean_width   = text_len_diff == 0 ? 0 : max_width - text_width;
        if (text_len_diff == 0) { break; }
        squeeze_and_extend_last_line(text_len_diff);
    }
    dirty_line_nr = -1;
}

void TextViewEngine::CursorPosition::reset() {
    pos = 0;
    row = 0;
    col = 0;
}

TextViewEngine::TextViewEngine(const QFontMetrics &metrics, int width)
    : fm(metrics) {
    text_ref = nullptr;
    reset_block_spacing(6.0);
    reset_line_spacing(1.0);
    reset(metrics, width);
}

void TextViewEngine::reset(const QFontMetrics &metrics, int width) {
    reset_font_metrics(fm);
    reset_max_width(width);
    cursor.reset();
    active_blocks.clear();
    active_block_index = -1;
    preedit            = false;
    preedit_text_ref   = nullptr;
}

TextBlock *TextViewEngine::alloc_block() {
    TextBlock *ptr = nullptr;
    if (block_pool.isEmpty()) {
        ptr = new TextBlock;
    } else {
        ptr = block_pool.front();
        block_pool.pop_front();
    }
    Q_ASSERT(ptr);
    return ptr;
}

bool TextViewEngine::is_empty() const {
    return active_blocks.isEmpty();
}

bool TextViewEngine::is_dirty() const {
    return dirty;
}

bool TextViewEngine::is_cursor_available() const {
    return active_block_index != -1;
}

void TextViewEngine::mark_as_dirty() {
    dirty = true;
}

TextLine &TextViewEngine::current_line() {
    Q_ASSERT(active_block_index != -1);
    return current_block()->lines[cursor.row];
}

const TextLine &TextViewEngine::current_line() const {
    Q_ASSERT(active_block_index != -1);
    return current_block()->lines[cursor.row];
}

TextBlock *TextViewEngine::current_block() {
    Q_ASSERT(active_block_index != -1);
    return active_blocks[active_block_index];
}

const TextBlock *TextViewEngine::current_block() const {
    Q_ASSERT(active_block_index != -1);
    return active_blocks[active_block_index];
}

void TextViewEngine::reset_max_width(int width) {
    max_width = width;
    for (auto &block : active_blocks) { block->mark_as_dirty(0); }
}

void TextViewEngine::reset_block_spacing(double spacing) {
    block_spacing = spacing;
}

void TextViewEngine::reset_line_spacing(double ratio) {
    line_spacing_ratio = ratio;
}

void TextViewEngine::reset_font_metrics(const QFontMetrics &metrics) {
    fm                  = metrics;
    standard_char_width = fm.horizontalAdvance(SAMPLE_CHAR);
    line_height         = fm.height() + fm.descent();
    for (auto block : active_blocks) { block->mark_as_dirty(0); }
}

void TextViewEngine::sync_cursor_row_col(int direction_hint) {
    const auto block = current_block();
    for (int i = 0; i < block->lines.size(); ++i) {
        if (const auto line = block->lines[i]; line.endp_offset >= cursor.pos) {
            cursor.row = i;
            cursor.col = cursor.pos;
            if (i > 0) {
                const auto last_line  = block->lines[i - 1];
                cursor.col           -= last_line.endp_offset;
            }
            if (direction_hint < 0 && cursor.col == line.text().length()
                && cursor.row + 1 < block->lines.size()) {
                ++cursor.row;
                cursor.col = 0;
            }
            return;
        }
    }
    Q_UNREACHABLE();
}

void TextViewEngine::render() {
    if (!dirty) { return; }
    if (!active_blocks.isEmpty()) {
        bool dirty_cursor = active_block_index != -1 && current_block()->is_dirty();
        for (auto block : active_blocks) {
            if (!block->is_dirty()) { continue; }
            block->render();
        };
        if (dirty_cursor) { sync_cursor_row_col(0); }
    }
    dirty = false;
}

void TextViewEngine::set_text_ref_unsafe(const QString *ref, int ref_origin) {
    text_ref         = ref;
    text_ref_origin  = ref_origin;
    preedit_text_ref = nullptr;
}

void TextViewEngine::clear_all() {
    active_blocks.clear();
    active_block_index = -1;
    cursor.reset();
    preedit = false;
    mark_as_dirty();
}

void TextViewEngine::insert_block(int index) {
    Q_ASSERT(text_ref);
    Q_ASSERT(index >= 0 && index <= active_blocks.size());
    int pos = -1;
    if (active_blocks.isEmpty()) {
        pos = 0;
    } else if (index == 0) {
        pos = active_blocks.front()->text_pos;
    } else {
        const auto &block = active_blocks[index - 1];
        pos               = block->text_pos + block->lines.back().endp_offset;
    }
    auto block    = alloc_block();
    block->parent = this;
    block->reset(text_ref, pos);
    active_blocks.insert(index, block);
    if (index <= active_block_index) { ++active_block_index; }
}

void TextViewEngine::break_block_at_cursor_pos() {
    if (is_empty() || active_block_index == -1) { return; }
    if (cursor.pos == 0) {
        //! NOTE: handle seperately to avoid expensive costs
        insert_block(active_block_index);
        return;
    }
    insert_block(active_block_index + 1);
    auto  block            = current_block();
    auto  next_block       = active_blocks[active_block_index + 1];
    auto &first_line       = next_block->lines.front();
    next_block->text_pos   = block->text_pos + cursor.pos;
    first_line.endp_offset = block->text_len() - cursor.pos;
    next_block->mark_as_dirty(0);
    if (cursor.col > 0) {
        block->lines[cursor.row].endp_offset = cursor.pos;
    } else {
        --cursor.row;
    }
    block->mark_as_dirty(cursor.row);
    block->lines.remove(cursor.row + 1, block->lines.size() - cursor.row - 1);
    ++active_block_index;
    cursor.pos = 0;
    sync_cursor_row_col(0);
}

void TextViewEngine::commit_insertion(int text_length) {
    Q_ASSERT(is_cursor_available());

    auto block = current_block();
    for (int i = cursor.row; i < block->lines.size(); ++i) {
        block->lines[i].endp_offset += text_length;
    }

    block->mark_as_dirty(cursor.row);
    cursor.pos += text_length;
    cursor.col += text_length;

    if (preedit) { return; }

    for (int i = active_block_index + 1; i < active_blocks.size(); ++i) {
        active_blocks[i]->text_pos += text_length;
    }
}

int TextViewEngine::commit_deletion(int times, int &deleted, bool hard_del) {
    Q_ASSERT(is_cursor_available());
    Q_ASSERT(!preedit);

    bool      cursor_moved = false;
    const int text_offset  = times < 0 ? commit_movement(times, &cursor_moved, hard_del) : 0;
    if (times < 0 && !cursor_moved) {
        deleted = 0;
        return 0;
    }

    int total_shift = 0;
    times           = qAbs(times);

    //! stage 1-1: delete within block
    //! stage 1-2: delete from cursor pos to end of the block
    auto block = current_block();
    if (const auto mean = block->text_len() - cursor.pos; times <= mean) {
        block->mark_as_dirty(cursor.row);
        block->join_dirty_lines();
        block->lines.back().endp_offset -= times;
        total_shift                     += times;
        times                            = 0;
    } else {
        block->lines.remove(cursor.row + 1, block->lines.size() - cursor.row - 1);
        block->lines.back().endp_offset  = cursor.pos;
        total_shift                     += mean;
        times                           -= mean;
    }

    //! stage 2: delete any complete blocks from the end of block
    int tail_block_index = active_block_index;
    while (tail_block_index + 1 < active_blocks.size()) {
        int stride = active_blocks[tail_block_index + 1]->text_len();
        if (!hard_del) { ++stride; }
        if (times < stride) { break; }
        times       -= stride;
        total_shift += stride;
        if (!hard_del) { --total_shift; }
        ++tail_block_index;
    }

    //! stage 3: join part of last deleted block into the current block
    if (cursor.pos == block->text_len() && times > 0
        && tail_block_index + 1 < active_blocks.size()) {
        const int len                = active_blocks[++tail_block_index]->text_len();
        int       next_block_del_len = times;
        if (!hard_del) { --next_block_del_len; }
        block->lines.back().endp_offset += len - next_block_del_len;
        total_shift                     += next_block_del_len;
    }
    active_blocks.remove(active_block_index + 1, tail_block_index - active_block_index);

    //! stage 4: sync following lines & blocks
    block->mark_as_dirty(cursor.row);
    for (int i = cursor.row + 1; i < block->lines.size(); ++i) {
        block->lines[i].endp_offset -= total_shift;
    }
    for (int i = active_block_index + 1; i < active_blocks.size(); ++i) {
        active_blocks[i]->text_pos -= total_shift;
    }

    deleted = total_shift;

    //! ATTENTION: mark as dirty would cause the cursor lost its direction hint when running render
    //! method by TextViewEngine, here we render the block in advance to keep the info of cursor
    //! FIXME: render per delete op could be costful, consider a better solution
    //! HINT: by checking the dirty cursor pos to guess the direction hint and render it in engine
    block->render();

    return text_offset;
}

int TextViewEngine::commit_movement(int offset, bool *moved, bool hard_move) {
    Q_ASSERT(is_cursor_available());
    Q_ASSERT(!preedit);

    const int last_active_block_index = active_block_index;
    const int last_cursor_pos         = cursor.pos;

    const int direction_hint  = offset;
    cursor.pos               += offset;

    //! NOTE: in soft mode, move between blocks cost one more movement

    while (true) {
        const auto len = current_block()->text_len();
        if (cursor.pos < 0 && active_block_index > 0) {
            cursor.pos += active_blocks[--active_block_index]->text_len();
            if (!hard_move) {
                ++cursor.pos;
                ++offset;
            }
            continue;
        }
        if (cursor.pos > len && active_block_index + 1 < active_blocks.size()) {
            cursor.pos -= active_blocks[active_block_index++]->text_len();
            if (!hard_move) {
                --cursor.pos;
                --offset;
            }
            continue;
        }
        const int old_pos  = cursor.pos;
        cursor.pos         = qBound(0, cursor.pos, len);
        offset            += cursor.pos - old_pos;
        break;
    }

    sync_cursor_row_col(direction_hint);

    if (moved) {
        *moved = last_active_block_index != active_block_index || last_cursor_pos != cursor.pos;
    }

    return offset;
}

void TextViewEngine::begin_preedit(QString &ref) {
    Q_ASSERT(is_cursor_available());
    Q_ASSERT(!preedit);

    auto block       = current_block();
    ref              = block->text().toString();
    preedit_text_ref = &ref;

    saved_text_pos    = block->text_pos;
    saved_text_length = block->text_len();
    saved_cursor      = cursor;
    block->text_ref   = preedit_text_ref;
    block->text_pos   = 0;
    preedit           = true;
}

void TextViewEngine::update_preedit_text(int text_length) {
    Q_ASSERT(preedit);
    Q_ASSERT(text_length >= 0);

    const int last_length = cursor.pos - saved_cursor.pos;
    const int offset      = text_length - last_length;
    auto      block       = current_block();

    block->mark_as_dirty(saved_cursor.row);
    block->join_dirty_lines();
    block->lines.back().endp_offset += offset;
    cursor.pos                      += offset;
}

void TextViewEngine::commit_preedit() {
    Q_ASSERT(preedit);

    auto block      = current_block();
    block->text_ref = text_ref;
    block->text_pos = saved_text_pos;
    cursor          = saved_cursor;
    preedit         = false;

    block->mark_as_dirty(cursor.row);
    block->join_dirty_lines();
    block->lines.back().endp_offset = saved_text_length;
}

int TextViewEngine::get_bounding_text_len(const QFontMetrics &fm, QStringView text, int &width) {
    int text_width = 0;
    int count      = 0;
    for (auto &c : text) {
        const auto char_width = fm.horizontalAdvance(c);
        if (text_width + char_width > width) { break; }
        text_width += char_width;
        ++count;
    }
    width = text_width;
    return count;
}

}; // namespace jwrite
