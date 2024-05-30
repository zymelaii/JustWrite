#pragma once

#include <jwrite/VisualTextEditContext.h>
#include <jwrite/TextInputCommand.h>
#include <jwrite/TextRestrictRule.h>
#include <jwrite/Tokenizer.h>
#include <QTimer>
#include <QWidget>
#include <atomic>

namespace jwrite::ui {

struct EditorPrivate;

class Editor : public QWidget {
    Q_OBJECT

public:
    virtual ~Editor();
    explicit Editor(QWidget *parent = nullptr);

signals:
    void textAreaChanged(QRect area);
    void textChanged(const QString &text);
    void focusLost(VisualTextEditContext::TextLoc last_loc);
    void activated();

public:
    bool softCenterMode() const;
    void setSoftCenterMode(bool value);

    QRect textArea() const;

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

    void insertRawText(const QString &text);
    bool insertedPairFilter(const QString &text);

    void select(int start_pos, int end_pos);
    void move(int offset, bool extend_sel);
    void moveTo(int pos, bool extend_sel);
    void insert(const QString &text);
    void del(int times);
    void copy();
    void cut();
    void paste();

    void breakIntoNewLine(bool should_update);
    void verticalMove(bool up);

    void setTimerEnabled(bool enabled);

    void setTextEngineLocked(bool locked) {
        busy_loading_ = locked;
    }

    void prepareRenderData() {
        setTextEngineLocked(true);
        context_->prepare_render_data();
        setTextEngineLocked(false);
    }

protected slots:
    void renderBlinkCursor();
    void render();

public:
    QSize minimumSizeHint() const override;
    QSize sizeHint() const override;

protected:
    void requestUpdate(bool sync);
    void setCursorShape(Qt::CursorShape shape);
    void restoreCursorShape();

    bool updateTextLocToVisualPos(const QPoint &vpos);
    void stopDragAndSelect();

    void drawTextArea(QPainter *p);
    void drawSelection(QPainter *p);
    void drawHighlightBlock(QPainter *p);
    void drawCursor(QPainter *p);

    void resizeEvent(QResizeEvent *e) override;
    void paintEvent(QPaintEvent *e) override;
    void focusInEvent(QFocusEvent *e) override;
    void focusOutEvent(QFocusEvent *e) override;
    void keyPressEvent(QKeyEvent *e) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
    void mouseDoubleClickEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void wheelEvent(QWheelEvent *e) override;
    void inputMethodEvent(QInputMethodEvent *e) override;
    void dragEnterEvent(QDragEnterEvent *e) override;
    void dropEvent(QDropEvent *e) override;

private:
    VisualTextEditContext    *context_;
    AbstractTextRestrictRule *restrict_rule_;
    TextInputCommandManager  *input_manager_;
    Tokenizer                *tokenizer_;

    int    min_text_line_chars_;
    bool   soft_center_mode_;
    bool   inserted_filter_enabled_;
    double expected_scroll_;

    bool   drag_sel_flag_;
    int    oob_drag_sel_flag_;
    QPoint oob_drag_sel_vpos_;

    bool   timer_enabled_;
    bool   update_requested_;
    QTimer stable_timer_;
    QTimer blink_timer_;
    bool   blink_cursor_should_paint_;

    bool   auto_scroll_mode_;
    double scroll_base_y_pos_;
    double scroll_ref_y_pos_;

    std::atomic_bool busy_loading_;

    QMargins        ui_margins_;
    Qt::CursorShape ui_cursor_shape_[2];
};

} // namespace jwrite::ui
