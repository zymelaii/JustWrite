#pragma once

#include <jwrite/VisualTextEditContext.h>
#include <jwrite/TextInputCommand.h>
#include <jwrite/TextEditHistory.h>
#include <jwrite/Tokenizer.h>
#include <jwrite/TextRestrictRule.h>
#include <QWidget>

namespace jwrite::ui {

class PrimaryEditor : public QWidget {
    Q_OBJECT

public:
    using CaretLoc = VisualTextEditContext::TextLoc;

    struct ViewBounds {
        int top;
        int bottom;
    };

    struct AutoScrollInfo {
        bool   enabled;
        double base_pos;
        double ref_pos;
    };

    enum class MouseActionState {
        Idle,
        PreAutoScroll,
        AutoScroll,
        LightAutoScroll,
        SetCaret,
        PreSelectText,
        SelectText,
        PreDragSelection,
        DragSelection,
        DragSelectionDone,
    };

    enum class CaretBlinkStyle {
        Solid,
        Blink,
        Expand,
        Smooth,
        Phase,
        Stack,
    };

    struct CaretBlinkState {
        bool            enabled;
        CaretBlinkStyle style;
        int             period;
        int             interval;
        double          progress;
    };

    struct TextDragState {
        bool     enabled;
        CaretLoc caret_loc;
    };

signals:
    void on_text_area_change(const QRect &rect);
    void on_text_change(const QString &text);
    void on_render_request();
    void on_activate();

public:
    [[nodiscard]] QString text() const;

    bool enabled() const;
    void set_enabled(bool on);

    bool readonly() const;
    void set_readonly(bool on);

    CaretLoc caret_loc() const;
    void     set_caret_loc(const CaretLoc &loc);
    void     set_caret_loc_to_pos(const QPoint &pos, bool extend_sel);

    CaretBlinkStyle caret_style() const;
    void            set_caret_style(CaretBlinkStyle style);

    bool insert_filter_enabled() const;
    void set_insert_filter_enabled(bool enabled);

    QRect text_area() const;

    double line_spacing() const;
    double line_spacing_ratio() const;
    void   set_line_spacing_ratio(double ratio);

    double block_spacing() const;
    void   set_block_spacing(double spacing);

    int  font_size() const;
    void set_font_size(int size);

    QFont text_font() const;
    void  set_text_font(QFont font);

    virtual ViewBounds recalc_view_bounds() const;
    void               request_scroll(double delta);
    void               request_scroll_to_pos(double pos);
    virtual void       process_scroll_request();

    int         horizontal_margin() const;
    virtual int horizontal_margin_hint() const;

    void del(int times);
    void insert(const QString &text, bool batch_mode);
    void select(int start_pos, int end_pos);
    void unselect();
    void move(int offset, bool extend_sel);
    void move_to(int pos, bool extend_sel);
    void vertical_move(bool up);
    void copy();
    void cut();
    void paste();
    void undo();
    void redo();

    bool         has_default_tic_handler(TextInputCommand cmd);
    virtual void execute_text_input_command(TextInputCommand cmd, const QString &text);

    bool test_pos_in_sel_area(const QPoint &pos) const;

    virtual Tokenizer                *tokenizer() const     = 0;
    virtual AbstractTextRestrictRule *restrict_rule() const = 0;

protected:
    virtual void draw(QPainter *p);
    virtual void draw_text(QPainter *p);
    virtual void draw_selection(QPainter *p);
    virtual void draw_caret(QPainter *p);

    VisualTextEditContext *context() const;
    TextViewEngine        *engine() const;
    TextEditHistory       *history() const;

    virtual bool insert_action_filter(const QString &text, bool batch_mode);
    virtual void internal_action_on_enter_block(int block_index);
    virtual void internal_action_on_leave_block(int block_index);

    void execute_delete_action(int times);
    void execute_insert_action(const QString &text, bool batch_mode);

private:
    void direct_remove_sel(QString *deleted_text);
    void direct_delete(int times, QString *deleted_text);
    void direct_insert(const QString &text);
    void direct_batch_insert(const QString &multiline_text);

    void request_update(bool sync);
    void save_and_set_cursor(Qt::CursorShape cursor);
    void restore_cursor();

    void complete_text_drag();

    void reset_mouse_action_state();

    void prepare_canvas();

    void reset_input_method_state();

    void handle_tic_insert_new_line();
    void handle_tic_insert_before_block();
    void handle_tic_insert_after_block();
    void handle_tic_cancel();
    void handle_tic_undo();
    void handle_tic_redo();
    void handle_tic_copy();
    void handle_tic_cut();
    void handle_tic_paste();
    void handle_tic_scroll_up();
    void handle_tic_scroll_down();
    void handle_tic_move_to_prev_char();
    void handle_tic_move_to_next_char();
    void handle_tic_move_to_prev_word();
    void handle_tic_move_to_next_word();
    void handle_tic_move_to_prev_line();
    void handle_tic_move_to_next_line();
    void handle_tic_move_to_start_of_line();
    void handle_tic_move_to_end_of_line();
    void handle_tic_move_to_start_of_block();
    void handle_tic_move_to_end_of_block();
    void handle_tic_move_to_start_of_document();
    void handle_tic_move_to_end_of_document();
    void handle_tic_move_to_prev_page();
    void handle_tic_move_to_next_page();
    void handle_tic_move_to_prev_block();
    void handle_tic_move_to_next_block();
    void handle_tic_delete_prev_char();
    void handle_tic_delete_next_char();
    void handle_tic_delete_prev_word();
    void handle_tic_delete_next_word();
    void handle_tic_delete_to_start_of_line();
    void handle_tic_delete_to_end_of_line();
    void handle_tic_select_prev_char();
    void handle_tic_select_next_char();
    void handle_tic_select_prev_word();
    void handle_tic_select_next_word();
    void handle_tic_select_to_prev_line();
    void handle_tic_select_to_next_line();
    void handle_tic_select_to_start_of_line();
    void handle_tic_select_to_end_of_line();
    void handle_tic_select_to_start_of_block();
    void handle_tic_select_to_end_of_block();
    void handle_tic_select_block();
    void handle_tic_select_prev_page();
    void handle_tic_select_next_page();
    void handle_tic_select_to_start_of_doc();
    void handle_tic_select_to_end_of_doc();
    void handle_tic_select_all();

public slots:
    void update_text_view_geometry();
    void render();

protected slots:
    void handle_on_text_area_change(const QRect &rect);
    void handle_caret_timer_on_timeout();
    void handle_prerender_timer_on_timeout();
    void handle_on_activate();

public:
    explicit PrimaryEditor(QWidget *parent = nullptr);
    ~PrimaryEditor() override = default;

    QSize minimumSizeHint() const override;
    QSize sizeHint() const override;

private:
    void init();
    void init_default_tic_handlers();

protected:
    bool     focusNextPrevChild(bool next) override;
    bool     event(QEvent *event) override;
    void     resizeEvent(QResizeEvent *event) override;
    void     paintEvent(QPaintEvent *event) override;
    void     focusInEvent(QFocusEvent *event) override;
    void     focusOutEvent(QFocusEvent *event) override;
    void     keyPressEvent(QKeyEvent *event) override;
    void     mousePressEvent(QMouseEvent *event) override;
    void     mouseReleaseEvent(QMouseEvent *event) override;
    void     mouseDoubleClickEvent(QMouseEvent *event) override;
    void     mouseMoveEvent(QMouseEvent *event) override;
    void     wheelEvent(QWheelEvent *event) override;
    void     dragEnterEvent(QDragEnterEvent *event) override;
    void     dropEvent(QDropEvent *event) override;
    void     inputMethodEvent(QInputMethodEvent *event) override;
    QVariant inputMethodQuery(Qt::InputMethodQuery query) const override;

private:
    std::unique_ptr<VisualTextEditContext>            context_;
    std::unique_ptr<TextEditHistory>                  history_;
    QMap<TextInputCommand, void (PrimaryEditor::*)()> default_tic_handlers_;

    MouseActionState ma_state_;

    bool            insert_filter_enabled_;
    bool            enabled_;
    bool            readonly_;
    bool            update_requested_;
    AutoScrollInfo  auto_scroll_info_;
    double          target_scroll_pos_;
    CaretBlinkState caret_blink_;
    TextDragState   text_drag_state_;
    QTimer         *prerender_timer_;
    QTimer         *caret_timer_;
    QFont           text_font_;
    QMargins        text_view_margins_;
    QPixmap         canvas_;
};

} // namespace jwrite::ui
