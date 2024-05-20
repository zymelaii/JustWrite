#pragma once

#include <jwrite/VisualTextEditContext.h>
#include <jwrite/TextInputCommand.h>
#include <QTimer>
#include <QWidget>

namespace jwrite::Ui {

struct EditorPrivate;

class Editor : public QWidget {
    Q_OBJECT

public:
    explicit Editor(QWidget *parent = nullptr);
    virtual ~Editor();

signals:
    void textAreaChanged(QRect area);
    void textChanged(const QString &text);
    void focusLost();
    void activated();

public:
    bool softCenterMode() const;
    void setSoftCenterMode(bool value);

    QRect textArea() const;

    void reset(QString &text, bool swap);
    void scrollToCursor();

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

    void breakIntoNewLine();
    void verticalMove(bool up);

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
    VisualTextEditContext           *context_;
    jwrite::TextInputCommandManager *input_manager_;

    int    min_text_line_chars_;
    bool   soft_center_mode_;
    bool   inserted_filter_enabled_;
    double expected_scroll_;

    bool   drag_sel_flag_;
    int    oob_drag_sel_flag_;
    QPoint oob_drag_sel_vpos_;

    bool   update_requested_;
    QTimer stable_timer_;

    QTimer blink_timer_;
    bool   blink_cursor_should_paint_;

    bool   auto_scroll_mode_;
    double scroll_base_y_pos_;
    double scroll_ref_y_pos_;

    QMargins        ui_margins_;
    Qt::CursorShape ui_cursor_shape_[2];
};

} // namespace jwrite::Ui