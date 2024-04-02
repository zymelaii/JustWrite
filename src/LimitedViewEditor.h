#pragma once

#include "TextBlockLayout.h"
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

public:
    int  paraBlockSpacing() const;
    void setParaBlockSpacing(int spacing);

    int  minTextLineChars() const;
    void setMinTextLineChars(int min_chars);

    double textLineSpacingRatio() const;
    void   setTextLineSpacingRatio(double ratio);

    bool alignCenter() const;
    void setAlignCenter(bool value);

    QRect textArea() const;

    TextBlockLayout &currentTextBlock();

    void switchTextBlock(int index);

    void insertMultiLineText(const QString &text);

    void scroll(double delta);

    void insert(const QString &text);
    void paste();
    void moveToPreviousChar();
    void moveToNextChar();
    void moveToEndOfLine();
    void moveToStartOfLine();
    void splitIntoNewLine();

protected:
    void updateTextBlockMaxWidth(int max_width);

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
