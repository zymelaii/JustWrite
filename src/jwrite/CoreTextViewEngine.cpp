#include <jwrite/CoreTextViewEngine.h>

namespace jwrite::core {

void CursorLoc::reset() {
    pos = 0;
    row = 0;
    col = 0;
}

QStringView RefTextLineView::text() const {
    Q_ASSERT(block);
    return block->text().mid(pos(), len());
}

int RefTextLineView::pos() const {
    Q_ASSERT(block);
    return is_first_line() ? 0 : block->at(line_nr - 1).endp_offset;
}

size_t RefTextLineView::len() const {
    return endp_offset - pos();
}

int RefTextLineView::max_width() const {
    Q_ASSERT(block);
    Q_ASSERT(block->engine);
    const int width = block->engine->viewport_width;
    return is_first_line() ? width - block->engine->indent_width() : width;
}

double RefTextLineView::spacing() const {
    return cached_extra_spacing * 1.0 / qMin<int>(1, len());
}

bool RefTextLineView::empty() const {
    return len() == 0;
}

bool RefTextLineView::dirty() const {
    Q_ASSERT(block);
    return block->dirty() && block->first_dirty_line_nr <= line_nr;
}

bool RefTextLineView::is_first_line() const {
    return line_nr == 0;
}

void RefTextLineView::mark_as_dirty() {
    Q_ASSERT(block);
    block->mark_as_dirty(line_nr);
}

int RefTextLineView::reshape() {
    Q_ASSERT(block);
    Q_ASSERT(block->engine);
    const int max_width = this->max_width();
    auto      text      = block->text().mid(pos());
    int       width     = max_width;
    const int old_len   = static_cast<int>(len());
    block->engine->get_bounding_text(text, width);
    const int new_len       = static_cast<int>(text.length());
    const int diff          = new_len - old_len;
    cached_tight_text_width = width;
    if (diff > 0 || diff == 0 && cached_extra_spacing == -1) {
        cached_extra_spacing = -1;
    } else {
        cached_extra_spacing = max_width - cached_tight_text_width;
    }
    endp_offset += diff;
    Q_ASSERT(!empty());
    return diff;
}

void RefTextBlockView::reset_ref(QString &text, int start_pos, size_t len) {
    Q_ASSERT(text.length() >= start_pos + len);
    text_ref      = std::addressof(text);
    ref_start_pos = start_pos;
    if (lines.empty()) {
        RefTextLineView line{
            .block                   = this,
            .line_nr                 = 0,
            .endp_offset             = static_cast<int>(len),
            .cached_tight_text_width = len == 0 ? 0 : -1,
            .cached_extra_spacing    = -1,
        };
        lines.append(line);
        if (len > 0) { mark_as_dirty(0); }
    } else {
        //! TODO: to be implemented
    }
}

QStringView RefTextBlockView::text() const {
    return QStringView(*text_ref).mid(ref_start_pos, len());
}

int RefTextBlockView::pos() const {
    //! NOTE: this is a costly operation, should be avoided if possible
    Q_UNIMPLEMENTED();
    return 0;
}

size_t RefTextBlockView::len() const {
    return last_line().endp_offset;
}

int RefTextBlockView::height() const {
    Q_ASSERT(engine);
    return empty() ? 0 : engine->line_spacing() + engine->line_stride() * lines.size();
}

RefTextLineView &RefTextBlockView::current_line() {
    Q_ASSERT(engine);
    Q_ASSERT(active());
    return at(engine->primary_cursor().row);
}

const RefTextLineView &RefTextBlockView::current_line() const {
    Q_ASSERT(engine);
    Q_ASSERT(active());
    return at(engine->primary_cursor().row);
}

RefTextLineView &RefTextBlockView::first_line() {
    return at(0);
}

const RefTextLineView &RefTextBlockView::first_line() const {
    return at(0);
}

RefTextLineView &RefTextBlockView::last_line() {
    return at(static_cast<int>(lines.size()) - 1);
}

const RefTextLineView &RefTextBlockView::last_line() const {
    return at(static_cast<int>(lines.size()) - 1);
}

RefTextLineView &RefTextBlockView::at(int line_nr) {
    Q_ASSERT(line_nr >= 0 && line_nr < static_cast<int>(lines.size()));
    return lines[line_nr];
}

const RefTextLineView &RefTextBlockView::at(int line_nr) const {
    Q_ASSERT(line_nr >= 0 && line_nr < static_cast<int>(lines.size()));
    return lines[line_nr];
}

bool RefTextBlockView::empty() const {
    return lines.empty();
}

bool RefTextBlockView::dirty() const {
    Q_ASSERT(
        first_dirty_line_nr == -1
        || first_dirty_line_nr >= 0 && first_dirty_line_nr < static_cast<int>(lines.size()));
    return first_dirty_line_nr != -1;
}

bool RefTextBlockView::active() const {
    Q_ASSERT(engine);
    return block_nr == engine->active_block_nr;
}

bool RefTextBlockView::is_first_block() const {
    return block_nr == 0;
}

bool RefTextBlockView::is_last_block() const {
    Q_ASSERT(engine);
    return block_nr == static_cast<int>(engine->total_blocks) - 1;
}

bool RefTextBlockView::is_baseline() const {
    Q_ASSERT(engine);
    return block_nr == engine->baseline_block_nr;
}

void RefTextBlockView::join_dirty_lines() {
    if (!dirty()) { return; }
    at(first_dirty_line_nr).endp_offset = last_line().endp_offset;
    const int total_lines               = static_cast<int>(lines.size());
    lines.remove(first_dirty_line_nr + 1, total_lines - (first_dirty_line_nr + 1));
}

void RefTextBlockView::mark_as_dirty(int line_nr) {
    if (!dirty() || line_nr < first_dirty_line_nr) { first_dirty_line_nr = line_nr; }
}

void RefTextBlockView::reshape() {
    if (!dirty()) { return; }
    join_dirty_lines();
    while (true) {
        auto     &line     = last_line();
        const int overflow = -line.reshape();
        Q_ASSERT(overflow >= 0);
        if (overflow == 0) { break; }
        RefTextLineView next_line{
            .block                   = this,
            .line_nr                 = static_cast<int>(lines.size()),
            .endp_offset             = line.endp_offset + overflow,
            .cached_tight_text_width = -1,
            .cached_extra_spacing    = -1,
        };
        lines.append(next_line);
    }
}

void CoreTextViewEngine::reset() {
    lock.lock_write();

    valid = true;

    preload_hint = 1;

    viewport_width  = 0;
    viewport_height = 0;

    viewport_pos           = 0;
    baseline_offset        = 0;
    total_height           = 0;
    total_height_change    = 0;
    global_space_available = false;

    block_spacing      = 0;
    line_spacing_ratio = 1.0;

    total_blocks      = 0;
    baseline_block_nr = 0;
    active_block_nr   = -1;
    active_blocks.clear();

    cursors.clear();

    CursorLoc loc{.block_nr = -1};
    loc.reset();
    cursors.append(loc);

    lock.unlock_write();
}

bool CoreTextViewEngine::is_viewport_invalid() const {
    return !valid;
}

void CoreTextViewEngine::invalidate_viewport() {
    valid = false;
}

void CoreTextViewEngine::resize_viewport(int width, int height) {
    Q_ASSERT(width > 0 && height > 0);

    lock.lock_write();
    do {
        const int width_diff  = width - viewport_width;
        const int height_diff = height - viewport_height;

        viewport_width  = width;
        viewport_height = height;

        if (total_blocks == 0) {
            invalidate_viewport();
            break;
        }

        if (width_diff != 0) {
            total_height_change    = 0;
            global_space_available = false;
        }

        if (width_diff != 0 || height_diff > 0) {
            invalidate_viewport();
            break;
        }
    } while (0);
    lock.unlock_write();
}

CursorLoc &CoreTextViewEngine::primary_cursor() {
    Q_ASSERT(!cursors.empty());
    return cursors.first();
}

const CursorLoc &CoreTextViewEngine::primary_cursor() const {
    Q_ASSERT(!cursors.empty());
    return cursors.first();
}

int CoreTextViewEngine::line_spacing() const {
    return metrics(TextViewMetrics::LineSpacing) * line_spacing_ratio;
}

int CoreTextViewEngine::line_height() const {
    return metrics(TextViewMetrics::LineHeight);
}

int CoreTextViewEngine::line_stride() const {
    return line_height() + line_spacing();
}

int CoreTextViewEngine::indent_width() const {
    return metrics(TextViewMetrics::IndentWidth);
}

void CoreTextViewEngine::render() {
    Q_UNIMPLEMENTED();

    if (!is_viewport_invalid()) { return; }

    lock.lock_write();

    Q_ASSERT(total_blocks > 0);
    Q_ASSERT(baseline_block_nr >= 0 && baseline_block_nr < total_blocks);

    //! stage 1: find the nearest block to the baseline block
    for (const auto block_ptr : active_blocks) { delete block_ptr; }
    active_blocks.clear();

    const int nearest_block_nr = baseline_block_nr;

    //! stage 2: get the viewport bounds
    QString          nearest_block_text = block_text(nearest_block_nr);
    RefTextBlockView nearest_block{
        .engine              = this,
        .block_nr            = nearest_block_nr,
        .first_dirty_line_nr = -1,
    };
    nearest_block.reset_ref(nearest_block_text, 0, nearest_block_text.length());
    nearest_block.reshape();

    const int rel_viewport_start = -baseline_offset;
    const int rel_viewport_end   = rel_viewport_start + viewport_height;
    const int rel_bound_start    = rel_viewport_start - viewport_height * preload_hint;
    const int rel_bound_end      = rel_viewport_end + viewport_height * preload_hint;
    Q_ASSERT(rel_viewport_start < rel_viewport_end);

    int block_nr_first = -1;
    int block_nr_last  = -1;

    int y_pos = baseline_offset;
    if (baseline_offset > rel_bound_end) {
        //! backward search
        Q_UNIMPLEMENTED();
    } else if (baseline_offset < rel_bound_start) {
        //! forward search
        Q_UNIMPLEMENTED();
        for (int i = nearest_block_nr; i < total_blocks; ++i) {
            QString          block_text = this->block_text(i);
            RefTextBlockView block{
                .engine              = this,
                .block_nr            = i,
                .first_dirty_line_nr = -1,
            };
            block.reset_ref(block_text, 0, block_text.length());
            block.reshape();
            const int block_start = y_pos;
            const int block_end   = block_start + block.height();
            if (block_start > rel_viewport_end) { break; }
            if (block_end < rel_viewport_start || block_start > rel_viewport_end) {}
        }
        Q_ASSERT(block_nr_first == -1);
        if (block_nr_last == -1) { block_nr_last = total_blocks - 1; }
    } else {
        //! bidirectional search
        int y_pos_backward = y_pos;
        for (int i = nearest_block_nr; i >= 0; --i) {
            QString          block_text = this->block_text(i);
            RefTextBlockView block{
                .engine              = this,
                .block_nr            = i,
                .first_dirty_line_nr = -1,
            };
            block.reset_ref(block_text, 0, block_text.length());
            block.reshape();
            const int block_end   = y_pos_backward - block_spacing;
            const int block_start = block_end - block.height();
            if (block_end < rel_bound_start) {
                block_nr_first = qMin(i + 1, nearest_block_nr);
                break;
            }
            y_pos_backward = block_start;
        }
        if (block_nr_first == -1) { block_nr_first = 0; }
        int y_pos_forward = y_pos;
        for (int i = nearest_block_nr; i < total_blocks; ++i) {
            QString          block_text = this->block_text(i);
            RefTextBlockView block{
                .engine              = this,
                .block_nr            = i,
                .first_dirty_line_nr = -1,
            };
            block.reset_ref(block_text, 0, block_text.length());
            block.reshape();
            const int block_start = y_pos_forward;
            const int block_end   = block_start + block.height();
            if (block_start > rel_bound_end) {
                block_nr_first = qMin(i + 1, nearest_block_nr);
                break;
            }
            y_pos_forward = block_end + block_spacing;
        }
        if (block_nr_last == -1) { block_nr_last = total_blocks - 1; }
    }

    //! stage 3: load active blocks
    for (int i = block_nr_first; i <= block_nr_last; ++i) {
        QString block_text         = this->block_text(i);
        auto    block              = new RefTextBlockView;
        block->engine              = this;
        block->block_nr            = i;
        block->first_dirty_line_nr = -1;
        block->reset_ref(block_text, 0, block_text.length());
        block->reshape();
        active_blocks.append(block);
    }

    lock.unlock_write();
}

void CoreTextViewEngine::execute_action(EditAction action, QVariant args) {
    Q_UNIMPLEMENTED();
}

void CoreTextViewEngine::get_bounding_text(QStringView &text, int &width) {
    const int max_width        = width;
    int       len              = 0;
    int       text_width       = 0;
    int       tight_text_width = 0;
    for (const auto &c : text) {
        const int char_width = this->text_width(QString(c));
        if (text_width + char_width > max_width) { break; }
        text_width       += horizontal_advance(QString(c));
        tight_text_width += char_width;
        ++len;
        if (text_width < max_width) { continue; }
        break;
    }
    text  = text.left(len);
    width = tight_text_width;
}

} // namespace jwrite::core
