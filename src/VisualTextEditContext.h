#pragma once

#include "TextViewEngine.h"

namespace jwrite {

struct VisualTextEditContext {
    struct Selection {
        int from;
        int to;

        bool empty() const {
            return from == to;
        }

        void clear() {
            from = to;
        }

        int len() {
            return qAbs(from - to);
        }
    };

    struct TextLoc {
        int block_index;
        int row;
        int col;
        int pos;
    };

    struct BlockRange {
        int first;
        int last;
    };

    //! NOTE: there may be some redundancy in the struct, but it's more convenient to keep them
    //! together
    struct CachedRenderState {
        bool              found_visible_block;
        double            first_visible_block_y_pos;
        bool              active_block_visible;
        double            active_block_y_start;
        double            active_block_y_end;
        BlockRange        visible_block;
        BlockRange        visible_sel;
        TextLoc           sel_loc_from;
        TextLoc           sel_loc_to;
        QMap<int, double> cached_block_y_pos;
    };

    TextViewEngine engine;

    int     edit_cursor_pos;
    QString edit_text;
    QString preedit_text;

    Selection sel;

    bool cursor_moved;

    bool   vertical_move_state;
    double vertical_move_ref_pos;

    double     viewport_y_pos;
    const int &viewport_width;
    int        viewport_height;

    bool              cached_render_data_ready;
    CachedRenderState cached_render_state;

    VisualTextEditContext(const QFontMetrics &fm, int width);

    void resize_viewport(int width, int height);
    void prepare_render_data();

    TextLoc current_textloc() const;
    TextLoc get_textloc_at_pos(int pos, int hint) const;

    /*!
     * \param [in] hint specify hint = 0 to use (row, col) member, otherwise use (pos) member and
     * hint will be seen as direction_hint
     */
    bool set_cursor_to_textloc(const TextLoc &loc, int hint);

    int     get_column_at_vpos(const TextLine &line, double x_pos) const;
    TextLoc get_textloc_at_vpos(const QPoint &pos) const;

    void begin_preedit();
    void update_preedit(const QString &text);
    void quit_preedit();
    void commit_preedit();

    bool has_sel() {
        return !sel.empty();
    }

    void unset_sel() {
        sel.clear();
    }

    void remove_sel_region();

    /*!
     * \param [in] hint expected movements to the further move command
     *
     * \return left movements after moving to one side of sel region
     *
     * \note move within sel region consumes one step of movement
     */
    int move_within_sel_region(int hint);

    void move(int offset, bool extend_sel);
    void move_to(int pos, bool extend_sel);
    void del(int times, bool hard_mode);
    void insert(const QString &text);

    bool vertical_move(bool up);
    void scroll_to(double pos);
};

}; // namespace jwrite
