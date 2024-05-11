#pragma once

#include <QWidget>

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

public:
    bool alignCenter() const;
    void setAlignCenter(bool value);

    QRect textArea() const;

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
    void splitIntoNewLine();

protected:
    void postUpdateRequest();

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
