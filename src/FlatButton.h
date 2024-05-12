
#pragma once

#include <QWidget>

class FlatButton : public QWidget {
    Q_OBJECT

public:
    explicit FlatButton(QWidget *parent = nullptr);
    virtual ~FlatButton() = default;

    QSize minimumSizeHint() const override;
    QSize sizeHint() const override;

    void setText(const QString &text);
    void setRadius(int radius);
    void setTextAlignment(int alignment);

signals:
    void pressed();

protected:
    void paintEvent(QPaintEvent *e) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *e) override;

private:
    QString text_;
    int     radius_;
    int     alignment_;
    bool    enter_;
};