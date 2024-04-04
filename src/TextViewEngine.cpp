#include "TextViewEngine.h"

namespace jwrite {

void TextLine::markAsDirty() const {
    parent->markAsDirty(line_nr);
}

QStringView TextLine::text() const {
    return parent->textOfLine(line_nr);
}

int TextLine::textOffset() const {
    return parent->offsetOfLine(line_nr);
}

double TextLine::charSpacing() const {
    const int len = endp_offset - textOffset();
    return len < 2 ? 0.0 : cached_mean_width * 1.0 / (len - 1);
}

bool TextLine::isFirstLine() const {
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

    markAsDirty(0);
}

void TextBlock::release() {
    //! ATTENTION: callee must remove block from active_blocks
    parent->block_pool.append(this);
}

void TextBlock::markAsDirty(int line_nr) {
    Q_ASSERT(line_nr >= 0 && line_nr < lines.size());
    dirty_line_nr = !isDirty() ? line_nr : qMin(dirty_line_nr, line_nr);
    parent->markAsDirty();
}

bool TextBlock::isDirty() const {
    Q_ASSERT(dirty_line_nr == -1 || dirty_line_nr >= 0);
    return dirty_line_nr != -1;
}

void TextBlock::joinDirtyLines() {
    Q_ASSERT(isDirty());
    lines[dirty_line_nr].endp_offset = lines.back().endp_offset;
    lines.remove(dirty_line_nr + 1, lines.size() - dirty_line_nr - 1);
}

int TextBlock::textLength() const {
    return lines.back().endp_offset;
}

int TextBlock::offsetOfLine(int index) const {
    Q_ASSERT(index >= 0 && index < lines.size());
    return index == 0 ? 0 : lines[index - 1].endp_offset;
}

int TextBlock::lengthOfLine(int index) const {
    Q_ASSERT(index >= 0 && index < lines.size());
    return lines[index].endp_offset - offsetOfLine(index);
}

QStringView TextBlock::textOfLine(int index) const {
    Q_ASSERT(index >= 0 && index < lines.size());
    Q_ASSERT(text_ref);
    const int offset = offsetOfLine(index);
    return QStringView(*text_ref).mid(text_pos + offset, lines[index].endp_offset - offset);
}

void TextBlock::squeezeAndExtendLastLine(int length) {
    Q_ASSERT(!lines.isEmpty());
    Q_ASSERT(length >= 0 && length < lengthOfLine(lines.size() - 1));
    TextLine line;
    line.parent               = this;
    line.line_nr              = lines.size();
    line.endp_offset          = lines.back().endp_offset;
    lines.back().endp_offset -= length;
    lines.push_back(line);
}

void TextBlock::render() {
    Q_ASSERT(isDirty());
    Q_ASSERT(!lines.isEmpty());
    joinDirtyLines();
    Q_ASSERT(dirty_line_nr + 1 == lines.size());
    const int leading_space_width = parent->standard_char_width * 2;
    while (true) {
        auto      &line       = lines.back();
        const auto text       = line.text();
        const int  max_width  = parent->max_width - (line.isFirstLine() ? leading_space_width : 0);
        int        text_width = max_width;
        const int  text_len   = TextViewEngine::boundingTextLength(parent->fm, text, text_width);
        const int  text_len_diff = text.length() - text_len;
        line.cached_text_width   = text_width;
        line.cached_mean_width   = text_len_diff == 0 ? 0 : max_width - text_width;
        if (text_len_diff == 0) { break; }
        squeezeAndExtendLastLine(text_len_diff);
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
    resetBlockSpacing(6.0);
    resetLineSpacing(1.0);
    reset(metrics, width);
}

void TextViewEngine::reset(const QFontMetrics &metrics, int width) {
    resetFontMetrics(fm);
    resetMaxWidth(width);
    cursor.reset();
    active_blocks.clear();
    active_block_index = -1;
    preedit            = false;
    preedit_text_ref   = nullptr;
}

TextBlock *TextViewEngine::allocBlock() {
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

bool TextViewEngine::isEmpty() const {
    return active_blocks.isEmpty();
}

bool TextViewEngine::isCursorAvailable() const {
    return active_block_index != -1;
}

void TextViewEngine::markAsDirty() {
    dirty = true;
}

TextLine &TextViewEngine::currentLine() {
    Q_ASSERT(active_block_index != -1);
    return currentBlock()->lines[cursor.row];
}

const TextLine &TextViewEngine::currentLine() const {
    Q_ASSERT(active_block_index != -1);
    return currentBlock()->lines[cursor.row];
}

TextBlock *TextViewEngine::currentBlock() {
    Q_ASSERT(active_block_index != -1);
    return active_blocks[active_block_index];
}

const TextBlock *TextViewEngine::currentBlock() const {
    Q_ASSERT(active_block_index != -1);
    return active_blocks[active_block_index];
}

void TextViewEngine::resetMaxWidth(int width) {
    max_width = width;
    for (auto &block : active_blocks) { block->markAsDirty(0); }
}

void TextViewEngine::resetBlockSpacing(double spacing) {
    block_spacing = spacing;
}

void TextViewEngine::resetLineSpacing(double ratio) {
    line_spacing_ratio = ratio;
}

void TextViewEngine::resetFontMetrics(const QFontMetrics &metrics) {
    fm                  = metrics;
    standard_char_width = fm.horizontalAdvance(SAMPLE_CHAR);
    line_height         = fm.height() + fm.descent();
    for (auto block : active_blocks) { block->markAsDirty(0); }
}

void TextViewEngine::syncCursorRowCol() {
    const auto block = currentBlock();
    for (int i = 0; i < block->lines.size(); ++i) {
        if (block->lines[i].endp_offset >= cursor.pos) {
            cursor.row = i;
            cursor.col = i == 0 ? cursor.pos : cursor.pos - block->lines[i - 1].endp_offset;
            return;
        }
    }
    Q_UNREACHABLE();
}

void TextViewEngine::render() {
    if (!dirty) { return; }
    if (!active_blocks.isEmpty()) {
        bool dirty_cursor = active_block_index != -1 && currentBlock()->isDirty();
        for (auto block : active_blocks) {
            if (!block->isDirty()) { continue; }
            block->render();
        };
        if (dirty_cursor) { syncCursorRowCol(); }
    }
    dirty = false;
}

void TextViewEngine::setTextRefUnsafe(const QString *ref, int ref_origin) {
    text_ref         = ref;
    text_ref_origin  = ref_origin;
    preedit_text_ref = nullptr;
}

void TextViewEngine::insertBlock(int index) {
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
    auto block    = allocBlock();
    block->parent = this;
    block->reset(text_ref, pos);
    active_blocks.insert(index, block);
    if (index <= active_block_index) { ++active_block_index; }
}

void TextViewEngine::breakBlockAtCursorPos() {
    if (isEmpty() || active_block_index == -1) { return; }
    if (cursor.pos == 0) {
        //! NOTE: handle seperately to avoid expensive costs
        insertBlock(active_block_index);
        return;
    }
    insertBlock(active_block_index + 1);
    auto  block            = currentBlock();
    auto  next_block       = active_blocks[active_block_index + 1];
    auto &first_line       = next_block->lines.front();
    next_block->text_pos   = block->text_pos + cursor.pos;
    first_line.endp_offset = block->textLength() - cursor.pos;
    next_block->markAsDirty(0);
    if (cursor.col > 0) {
        block->lines[cursor.row].endp_offset = cursor.pos;
    } else {
        --cursor.row;
    }
    block->markAsDirty(cursor.row);
    block->lines.remove(cursor.row + 1, block->lines.size() - cursor.row - 1);
    ++active_block_index;
    cursor.pos = 0;
    syncCursorRowCol();
}

void TextViewEngine::commitInsertion(int text_length) {
    Q_ASSERT(isCursorAvailable());
    Q_ASSERT(!preedit);
    auto block = currentBlock();
    for (int i = cursor.row; i < block->lines.size(); ++i) {
        block->lines[i].endp_offset += text_length;
    }
    for (int i = active_block_index + 1; i < active_blocks.size(); ++i) {
        active_blocks[i]->text_pos += text_length;
    }
    block->markAsDirty(cursor.row);
    cursor.pos += text_length;
    cursor.col += text_length;
}

int TextViewEngine::commitDeletion(int times, int &deleted) {
    Q_ASSERT(isCursorAvailable());
    Q_ASSERT(!preedit);

    bool      cursor_moved = false;
    const int text_offset  = times < 0 ? commitMovement(times, &cursor_moved) : 0;
    if (times < 0 && !cursor_moved) {
        deleted = 0;
        return 0;
    }

    int total_shift = 0;
    times           = qAbs(times);

    //! stage 1-1: delete within block
    //! stage 1-2: delete from cursor pos to end of the block
    auto block = currentBlock();
    if (const auto mean = block->textLength() - cursor.pos; times <= mean) {
        block->markAsDirty(cursor.row);
        block->joinDirtyLines();
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
        const auto stride = active_blocks[tail_block_index + 1]->textLength() + 1;
        if (times < stride) { break; }
        times       -= stride;
        total_shift += stride - 1;
        ++tail_block_index;
    }

    //! stage 3: join part of last deleted block into the current block
    if (cursor.pos == block->textLength() && times > 0
        && tail_block_index + 1 < active_blocks.size()) {
        const auto len                   = active_blocks[++tail_block_index]->textLength();
        block->lines.back().endp_offset += len - times + 1;
        total_shift                     += times - 1;
    }
    active_blocks.remove(active_block_index + 1, tail_block_index - active_block_index);

    //! stage 4: sync following lines & blocks
    block->markAsDirty(cursor.row);
    for (int i = cursor.row + 1; i < block->lines.size(); ++i) {
        block->lines[i].endp_offset -= total_shift;
    }
    for (int i = active_block_index + 1; i < active_blocks.size(); ++i) {
        block->text_pos -= total_shift;
    }

    deleted = total_shift;

    return text_offset;
}

int TextViewEngine::commitMovement(int offset, bool *moved) {
    Q_ASSERT(isCursorAvailable());
    Q_ASSERT(!preedit);

    const int last_active_block_index = active_block_index;
    const int last_cursor_pos         = cursor.pos;

    cursor.pos += offset;

    //! NOTE: move between blocks cost one more movement

    while (true) {
        const auto len = currentBlock()->textLength();
        if (cursor.pos < 0 && active_block_index > 0) {
            cursor.pos += active_blocks[--active_block_index]->textLength() + 1;
            ++offset;
            continue;
        }
        if (cursor.pos > len && active_block_index + 1 < active_blocks.size()) {
            cursor.pos -= active_blocks[active_block_index++]->textLength() + 1;
            --offset;
            continue;
        }
        const int old_pos  = cursor.pos;
        cursor.pos         = qBound(0, cursor.pos, len);
        offset            += cursor.pos - old_pos;
        break;
    }

    syncCursorRowCol();

    //! NOTE: `syncCursorRowCol` always place the pos in a smaller row, correct it if possible
    if (auto block = currentBlock(); offset < 0 && cursor.col == block->lengthOfLine(cursor.row)
                                     && cursor.row + 1 < block->lines.size()) {
        ++cursor.row;
        cursor.col = 0;
    }

    if (moved) {
        *moved = last_active_block_index != active_block_index || last_cursor_pos != cursor.pos;
    }

    return offset;
}

int TextViewEngine::boundingTextLength(const QFontMetrics &fm, QStringView text, int &width) {
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

QDebug operator<<(QDebug dbg, const jwrite::TextViewEngine::CursorPosition &pos) {
    dbg.nospace() << "(" << pos.pos << "," << pos.row << ":" << pos.col << ")";
    return dbg.maybeSpace();
}
