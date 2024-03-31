#pragma once

#include <QWidget>

struct ParaBlockEditPrivate;

class ParaBlockEdit : public QWidget {
    Q_OBJECT

public:
    explicit ParaBlockEdit(QWidget *parent = nullptr);
    virtual ~ParaBlockEdit();

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

    void     setCursorPosition(int pos);
    int      cursorPosition() const;
    void     setText(const QString &text);
    QString  text() const;
    QString  selectedText() const;
    void     clearText();
    void     setTextMargins();
    QMargins textMargins() const;
    bool     hasSelText() const;
    void     clearSelText();
    void     unsetSelection();
    int      getTextLengthInWidth(QStringView text, int width, int *text_width) const;

    void insert(const QString &text);
    void del();
    void backspace();
    void select(int begin, int end);
    void selectAll();
    void copy();
    void paste();
    void cut();
    void moveLeft();
    void moveRight();
    void moveDown();
    void moveUp();
    void gotoStartOfBlock();
    void gotoEndOfBlock();

protected:
    void     resizeEvent(QResizeEvent *e) override;
    void     paintEvent(QPaintEvent *e) override;
    void     focusInEvent(QFocusEvent *e) override;
    void     focusOutEvent(QFocusEvent *e) override;
    void     keyPressEvent(QKeyEvent *e) override;
    void     inputMethodEvent(QInputMethodEvent *e) override;
    QVariant inputMethodQuery(Qt::InputMethodQuery query) const override;

    bool isAcceptableInput(QKeyEvent *e);
    int  getTightLineHeight() const;

signals:
    void textChanged(const QString &text, const QString &old_text);
    void displayTextChanged(const QString &text);
    void cursorPositionChanged(int pos);
    void selTextChanged(const QString &text, int sel_begin, int sel_end);
    void finished();
    void gotFocus();
    void lostFocus();

protected slots:
    void updateDisplayText(const QString &text);
    void postUpdateRequest();
    void adjustTextRegion();

private:
    ParaBlockEditPrivate *d;
};
