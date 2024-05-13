#pragma once

#include <QWidget>

struct EditorTextLoc {
    int block_index;
    //! NOTE: only one of the {row, col}/{pos} is valid
    int row;
    int col;
    int pos;
};

struct LimitedViewEditorPrivate;

class LimitedViewEditor : public QWidget {
    Q_OBJECT

public:
    explicit LimitedViewEditor(QWidget *parent = nullptr);
    virtual ~LimitedViewEditor();

    QSize minimumSizeHint() const override;
    QSize sizeHint() const override;

signals:
    void textAreaChanged(QRect area);
    void textChanged(const QString &text);
    void focusLost();
    void requireEmptyChapter();

public:
    bool alignCenter() const;
    void setAlignCenter(bool value);

    QRect textArea() const;

    EditorTextLoc textLoc() const;
    EditorTextLoc textLocAtPos(int pos) const;
    void          setTextLoc(EditorTextLoc loc);

    void reset(QString &text, bool swap);
    void cancelPreedit();
    void scrollToCursor();

    void scroll(double delta, bool smooth);
    void scrollToStart();
    void scrollToEnd();

    void insertDirtyText(const QString &text);
    bool insertedPairFilter(const QString &text);

    void move(int offset);
    void insert(const QString &text);
    void del(int times);
    void copy();
    void cut();
    void paste();

    bool hasSelection() const;
    void unsetSelection();
    void select(int from, int to);
    void expandSelection(int offset);
    void expandSelectionTo(int pos);

    void splitIntoNewLine();

protected:
    EditorTextLoc getTextLocAtPos(const QPoint &pos);
    void          postUpdateRequest();

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
    LimitedViewEditorPrivate *d;
};
