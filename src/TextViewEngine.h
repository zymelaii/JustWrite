#pragma once

#include <QFontMetrics>
#include <QVector>
#include <QString>
#include <QStringView>
#include <QList>
#include <QDebug>

namespace jwrite {

struct TextBlock;
struct TextViewEngine;

struct TextLine {
    TextBlock *parent;
    int        line_nr;
    int        endp_offset;
    int        cached_text_width;
    int        cached_mean_width;

    void        mark_as_dirty() const;
    QStringView text() const;
    int         text_len() const;
    int         text_offset() const;
    double      char_spacing() const;
    bool        is_first_line() const;
};

struct TextBlock {
    TextViewEngine *parent;
    const QString  *text_ref;
    int             text_pos;

    QVector<TextLine> lines;

    int dirty_line_nr;

    void            reset(const QString *ref, int pos);
    void            release();
    void            mark_as_dirty(int line_nr);
    bool            is_dirty() const;
    void            join_dirty_lines();
    int             text_len() const;
    int             offset_of_line(int index) const;
    int             len_of_line(int index) const;
    TextLine       &current_line();
    const TextLine &current_line() const;
    QStringView     text_of_line(int index) const;
    QStringView     text() const;
    void            squeeze_and_extend_last_line(int length);
    void            render();
};

struct TextViewEngine {
    constexpr static QChar SAMPLE_CHAR = QChar(U'\u3000');

    QFontMetrics fm;
    int          standard_char_width;
    int          max_width;

    int                  active_block_index;
    QVector<TextBlock *> active_blocks;

    int    line_height;
    double block_spacing;
    double line_spacing_ratio;

    struct CursorPosition {
        int pos;
        int row;
        int col;

        void reset();
    } cursor;

    int            text_ref_origin;
    const QString *text_ref;

    bool           preedit;
    CursorPosition saved_cursor;
    int            saved_text_pos;
    int            saved_text_length;
    const QString *preedit_text_ref;

    bool dirty;

    QList<TextBlock *> block_pool;

    TextViewEngine(const QFontMetrics &metrics, int width);

    void             reset(const QFontMetrics &metrics, int width);
    TextBlock       *alloc_block();
    bool             is_empty() const;
    bool             is_dirty() const;
    bool             is_cursor_available() const;
    void             mark_as_dirty();
    TextLine        &current_line();
    const TextLine  &current_line() const;
    TextBlock       *current_block();
    const TextBlock *current_block() const;
    void             reset_max_width(int width);
    void             reset_block_spacing(double spacing);
    void             reset_line_spacing(double ratio);
    void             reset_font_metrics(const QFontMetrics &metrics);
    void             sync_cursor_row_col(int direction_hint);
    void             render();

    //! TODO: promote unsafe method into the safe one
    void set_text_ref_unsafe(const QString *ref, int ref_origin);
    void clear_all();
    void insert_block(int index);
    void break_block_at_cursor_pos();
    void commit_insertion(int text_length);
    int  commit_deletion(int times, int &deleted, bool hard_del);
    int  commit_movement(int offset, bool *moved, bool hard_move);
    void begin_preedit(QString &ref);
    void update_preedit_text(int text_length);
    void commit_preedit();

    static int get_bounding_text_len(const QFontMetrics &fm, QStringView text, int &width);
};

}; // namespace jwrite
