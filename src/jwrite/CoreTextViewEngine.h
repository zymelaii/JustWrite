#pragma once

#include <QStringView>
#include <QVariant>
#include <QList>
#include <atomic>
#include <stddef.h>

namespace jwrite::core {

class RwLock {
public:
    RwLock();
    RwLock(const RwLock &)            = delete;
    RwLock &operator=(const RwLock &) = delete;

    bool try_lock_read();
    void lock_read();
    void unlock_read();

    bool try_lock_write();
    void lock_write();
    void unlock_write();

    bool on_read() const;
    bool on_write() const;

private:
    std::atomic_int state_;
};

struct RefTextBlockView;
struct CoreTextViewEngine;

struct EmphasisMark {
    int pos;
    int len;
    int type;
};

enum class EditAction {
    Insert,
    Delete,
    Move,
};

enum class TextViewMetrics {
    IndentWidth,
    LineSpacing,
    LineHeight,
    StandardCharWidth,
};

struct CursorLoc {
    int block_nr;
    int pos;
    int row;
    int col;

    void reset();
};

struct RefTextLineView {
    RefTextBlockView *block;

    int line_nr;
    int endp_offset;

    int cached_tight_text_width;
    int cached_extra_spacing;

    /*!
     * \brief get the text of the line
     */
    QStringView text() const;

    /*!
     * \brief get the start pos of the line in the current block
     */
    int pos() const;

    /*!
     * \brief get the length of the text in the line
     */
    size_t len() const;

    /*!
     * \brief get the max text width of the line in pixels
     *
     * \note attributes such as first line or not have effect on the max width
     */
    int max_width() const;

    /*!
     * \brief get the char spacing of the line in pixels
     */
    double spacing() const;

    /*!
     * \brief check if the line is empty
     */
    bool empty() const;

    /*!
     * \brief check if the line is dirty
     *
     * \note the opposite state of dirty is ready
     */
    bool dirty() const;

    /*!
     * \brief check if the line is the first line of the block
     */
    bool is_first_line() const;

    /*!
     * \brief mark the line as dirty
     */
    void mark_as_dirty();

    /*!
     * \brief reshape the line view and mark as ready
     *
     * \return change of the text length after reshape
     */
    int reshape();
};

struct RefTextBlockView {
    CoreTextViewEngine *engine;

    int block_nr;
    int first_dirty_line_nr;

    const QString *text_ref;
    int            ref_start_pos;

    QList<RefTextLineView> lines;
    QList<EmphasisMark>    emphases;

    /*!
     * \brief reset the reference of the block to a new text
     */
    void reset_ref(QString &text, int start_pos, size_t len);

    /*!
     * \brief get the text of the block
     */
    QStringView text() const;

    /*!
     * \brief get the start pos of the block in the whole text
     */
    int pos() const;

    /*!
     * \brief get the length of the text in the block
     */
    size_t len() const;

    /*!
     * \brief get the text height of the block in pixels
     *
     * \note using the height of a dirty block is a undefined behavior, but you can keep going as
     * long as you know what you're doing
     */
    int height() const;

    /*!
     * \brief get the current line of the block
     *
     * \note caller must ensure that the block is active
     */
    RefTextLineView       &current_line();
    const RefTextLineView &current_line() const;

    /*!
     * \brief get the first line of the block
     *
     * \note caller must ensure that the block is not empty
     * \note access to the first line of a dirty block is a undefined behavior, but you can keep
     * going as long as you know what you're doing
     */
    RefTextLineView       &first_line();
    const RefTextLineView &first_line() const;

    /*!
     * \brief get the last line of the block
     *
     * \note caller must ensure that the block is not empty
     * \note access to the first line of a dirty block is a undefined behavior, but you can keep
     * going as long as you know what you're doing
     */
    RefTextLineView       &last_line();
    const RefTextLineView &last_line() const;

    /*!
     * \brief get the line at the given line number
     *
     * \note caller must ensure that the given line number is valid
     */
    RefTextLineView       &at(int line_nr);
    const RefTextLineView &at(int line_nr) const;

    /*!
     * \brief check if the block is empty
     */
    bool empty() const;

    /*!
     * \brief check if the block is dirty
     */
    bool dirty() const;

    /*!
     * \brief check if the block is active
     */
    bool active() const;

    /*!
     * \brief check if the block is the first block of the text
     */
    bool is_first_block() const;

    /*!
     * \brief check if the block is the last block of the text
     */
    bool is_last_block() const;

    /*!
     * \brief check if the block is the baseline block in the text view engine
     */
    bool is_baseline() const;

    /*!
     * \brief join the dirty lines into the first dirty line of the block
     */
    void join_dirty_lines();

    /*!
     * \brief mark the block as dirty and update the first dirty line number
     */
    void mark_as_dirty(int line_nr);

    /*!
     * \brief reshape the block view and mark as ready
     */
    void reshape();
};

struct CoreTextViewEngine {
    RwLock lock;
    bool   valid;

    //! hint of number of blocks to preload
    //! \note load preload_hint*2+1 pages of blocks at most
    int preload_hint;

    //! width of the viewport in pixels
    int viewport_width;

    //! width of the viewport in pixels
    int viewport_height;

    //! vertical position of the viewport in the whole text view in pixels
    int viewport_pos;

    //! vertical offset of the baseline block relative to the viewport in pixels
    int baseline_offset;

    //! total height of the text view in pixels
    int total_height;

    //! change of the total height of the text view in pixels
    int total_height_change;

    //! whether the global space scale is available, includes `viewport_pos` and `total_height`
    bool global_space_available;

    //! spacing between blocks in pixels
    int block_spacing;

    //! ratio of line spacing to the spacing provided by the `metrics`
    double line_spacing_ratio;

    //! total blocks in the whole text view
    size_t total_blocks;

    //! number of the baseline block
    //! \note baseline block is always available in the `active_blocks`
    int baseline_block_nr;

    //! number of the active block
    //! \note the active block is the block that the primary cursor is currently in
    //! \note -1 means no block is active
    //! \note the value is always equals to the block number represented by the primary cursor
    int active_block_nr;

    //! collection of continuously active blocks
    QList<RefTextBlockView *> active_blocks;

    //! collection of cursors
    //! \note a cursor is where the action is applied to
    //! \note the primary cursor is always the first cursor in the list
    QList<CursorLoc> cursors;

    void reset();

    bool is_viewport_invalid() const;
    void invalidate_viewport();

    void resize_viewport(int width, int height);

    CursorLoc       &primary_cursor();
    const CursorLoc &primary_cursor() const;

    int line_spacing() const;
    int line_height() const;
    int line_stride() const;
    int indent_width() const;

    void render();

    void execute_action(EditAction action, QVariant args);

    virtual QString block_text(int block_nr) const = 0;

    virtual int metrics(TextViewMetrics type) const        = 0;
    virtual int horizontal_advance(QStringView text) const = 0;
    virtual int text_width(QStringView text) const         = 0;

    /*!
     * \param [inout] text input whole text, output bouding text
     * \param [inout] width input max width, output tight text width
     */
    void get_bounding_text(QStringView &text, int &width);
};

} // namespace jwrite::core
