#pragma once

#include <jwrite/VisualTextEditContext.h>
#include <jwrite/TextInputCommand.h>
#include <jwrite/TextRestrictRule.h>
#include <jwrite/TextEditHistory.h>
#include <jwrite/Tokenizer.h>
#include <jwrite/AppConfig.h>
#include <QTimer>
#include <QWidget>

namespace jwrite::ui {

struct EditorPrivate;

class Editor : public QWidget {
    Q_OBJECT

public:
    enum class AutoCentre {
        Never,
        Always,
        Adaptive,
    };

signals:
    void on_text_area_change(QRect area);
    void textChanged(const QString &text);
    void focusLost(VisualTextEditContext::TextLoc last_loc);
    void activated();

public slots:

    void set_auto_centre_mode(AutoCentre mode) {
        auto_centre_edit_line_ = mode;
    }

    void set_text_focus_mode(AppConfig::TextFocusMode mode) {
        focus_mode_ = mode;
        update();
    }

    void set_unfocused_text_opacity(double opacity) {
        unfocused_text_opacity_ = opacity;
        update();
    }

    void set_line_spacing_ratio(double ratio) {
        context_->engine.line_spacing_ratio = ratio;
        context_->cached_render_data_ready  = false;
        update();
    }

    void set_block_spacing(double spacing) {
        context_->engine.block_spacing     = spacing;
        context_->cached_render_data_ready = false;
        update();
    }

    void set_font_size(int size) {
        ui_content_font_.setPointSize(size);
        context_->engine.reset_font_metrics(QFontMetrics(ui_content_font_));
        update();
    }

    void set_font(QFont font) {
        const int size   = ui_content_font_.pointSize();
        ui_content_font_ = font;
        ui_content_font_.setPointSize(size);
        context_->engine.reset_font_metrics(QFontMetrics(ui_content_font_));
        update();
    }

    void set_smooth_scroll_enabled(bool enabled) {
        smooth_scroll_enabled_ = enabled;
    }

public:
    bool soft_center_mode_enabled() const;
    void set_soft_center_mode_enabled(bool value);

    bool elastic_resize_enabled() const;
    void set_elastic_resize_enabled(bool value);

    void update_text_view_margins();

    QRect text_area() const;

    void    reset(QString &text, bool swap);
    QString take();
    void    scrollToCursor();

    QString text() const;

    VisualTextEditContext::TextLoc currentTextLoc() const;
    void                           setCursorToTextLoc(const VisualTextEditContext::TextLoc &loc);

    QPair<double, double> scrollBound() const;
    void                  scroll(double delta, bool smooth);
    void                  scrollTo(double pos_y, bool smooth);
    void                  scrollToStart();
    void                  scrollToEnd();

    /*!
     * \return expected horizontal margin hint in pixels
     *
     * \note calc one side margin requires a division by 2, and this would lose precision in some
     * cases, so let `h-margin = left-margin + right-margin` and recalc `left-margin = h-margin /
     * 2`, `right-margin = h-margin - left-margin`
     */
    int smart_margin_hint() const;

    /*!
     * \return horizontal margin in pixels
     */
    int smart_margin() const;

    void direct_remove_sel(QString *deleted_text);
    void direct_delete(int times, QString *deleted_text);
    void execute_delete_action(int times);
    void direct_insert(const QString &text);
    void direct_batch_insert(const QString &multiline_text);
    void execute_insert_action(const QString &text, bool batch_mode);
    bool insert_action_filter(const QString &text);

    void del(int times);
    void insert(const QString &text, bool batch_mode);
    void select(int start_pos, int end_pos);
    void move(int offset, bool extend_sel);
    void move_to(int pos, bool extend_sel);
    void copy();
    void cut();
    void paste();
    void undo();
    void redo();

    [[deprecated]] void breakIntoNewLine(bool should_update);
    void                verticalMove(bool up);

    Tokenizer *tokenizer() const;

    void setTimerEnabled(bool enabled);

    void prepare_render_data() {
        context_->prepare_render_data();
    }

    auto lock_guard() const {
        return context_->lock.write_lock_guard();
    }

protected slots:
    void renderBlinkCursor();
    void render();

public:
    explicit Editor(QWidget *parent = nullptr);
    ~Editor() override;

public:
    QSize minimumSizeHint() const override;
    QSize sizeHint() const override;

protected:
    void init();

    void requestUpdate(bool sync);
    void setCursorShape(Qt::CursorShape shape);
    void restoreCursorShape();

    bool updateTextLocToVisualPos(const QPoint &vpos);
    void stopDragAndSelect();

    void drawTextArea(QPainter *p);
    void drawSelection(QPainter *p);
    void drawHighlightBlock(QPainter *p);
    void drawCursor(QPainter *p);

    void     resizeEvent(QResizeEvent *e) override;
    void     paintEvent(QPaintEvent *e) override;
    void     focusInEvent(QFocusEvent *e) override;
    void     focusOutEvent(QFocusEvent *e) override;
    void     keyPressEvent(QKeyEvent *e) override;
    void     mousePressEvent(QMouseEvent *e) override;
    void     mouseReleaseEvent(QMouseEvent *e) override;
    void     mouseDoubleClickEvent(QMouseEvent *e) override;
    void     mouseMoveEvent(QMouseEvent *e) override;
    void     wheelEvent(QWheelEvent *e) override;
    void     dragEnterEvent(QDragEnterEvent *e) override;
    void     dropEvent(QDropEvent *e) override;
    void     inputMethodEvent(QInputMethodEvent *e) override;
    QVariant inputMethodQuery(Qt::InputMethodQuery query) const override;

private:
    VisualTextEditContext    *context_;
    AbstractTextRestrictRule *restrict_rule_;
    Tokenizer                *tokenizer_;
    QFuture<Tokenizer *>      fut_tokenizer_;
    TextEditHistory           history_;

    std::optional<VisualTextEditContext::TextLoc> last_text_loc_;

    int                      min_text_line_chars_;
    bool                     soft_center_mode_;
    bool                     elastic_resize_;
    bool                     inserted_filter_enabled_;
    double                   expected_scroll_;
    AutoCentre               auto_centre_edit_line_;
    AppConfig::TextFocusMode focus_mode_;
    double                   unfocused_text_opacity_;

    bool   drag_sel_flag_;
    int    oob_drag_sel_flag_;
    QPoint oob_drag_sel_vpos_;

    bool   timer_enabled_;
    bool   update_requested_;
    QTimer stable_timer_;
    QTimer blink_timer_;
    bool   blink_cursor_should_paint_;

    bool   smooth_scroll_enabled_;
    bool   auto_scroll_mode_;
    double scroll_base_y_pos_;
    double scroll_ref_y_pos_;

    QFont           ui_content_font_;
    QMargins        ui_margins_;
    Qt::CursorShape ui_cursor_shape_[2];
};

} // namespace jwrite::ui
