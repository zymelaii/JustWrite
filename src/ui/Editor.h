#pragma once

#include <QWidget>

namespace jwrite::Ui {

struct EditorTextLoc {
    int block_index;
    //! NOTE: only one of the {row, col}/{pos} is valid
    int row;
    int col;
    int pos;
};

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
    void requireEmptyChapter();

public:
    bool is_soft_center_mode() const;
    void set_soft_center_mode(bool value);

    QRect get_text_area() const;

    EditorTextLoc get_current_text_loc() const;
    EditorTextLoc get_text_loc_at_pos(int pos) const;
    void          update_text_loc(EditorTextLoc loc);

    void reset(QString &text, bool swap);
    void cancel_preedit();
    void scroll_to_cursor();

    void scroll(double delta, bool smooth);
    void scoll_to_start();
    void scroll_to_end();

    void insert_raw_text(const QString &text);
    bool inserted_pair_filter(const QString &text);

    void move(int offset, bool extend_sel);
    void move_to(int pos, bool extend_sel);
    void insert(const QString &text);
    void del(int times);
    void copy();
    void cut();
    void paste();

    bool            has_sel() const;
    void            unset_sel();
    QPair<int, int> sel_region() const;

    void break_into_newline();
    void visual_vertical_move(bool up);

protected:
    EditorTextLoc get_text_loc_at_vpos(const QPoint &pos);
    void          request_update();
    void          set_cursor_shape(Qt::CursorShape shape);
    void          restore_cursor_shape();

public:
    QSize minimumSizeHint() const override;
    QSize sizeHint() const override;

protected:
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
    EditorPrivate *d;
};

} // namespace jwrite::Ui
