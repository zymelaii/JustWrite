#pragma once

#include <QWidget>

namespace jwrite::Ui {

class FlatButton : public QWidget {
    Q_OBJECT

public:
    explicit FlatButton(QWidget *parent = nullptr);
    virtual ~FlatButton();

signals:
    void pressed();

public:
    QString text() const {
        return text_;
    }

    void setText(const QString &text);

    int radius() const {
        return ui_radius_;
    }

    void setRadius(int radius);

    Qt::Alignment textAlignment() {
        return ui_alignment_;
    }

    void setTextAlignment(Qt::Alignment alignment);

public:
    QSize minimumSizeHint() const override;
    QSize sizeHint() const override;

protected:
    void setupUi();

    void paintEvent(QPaintEvent *e) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *e) override;

private:
    QString text_;

    int           ui_radius_;
    Qt::Alignment ui_alignment_;
    bool          ui_mouse_entered_;
};

} // namespace jwrite::Ui
