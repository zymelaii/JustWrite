#pragma once

#include <QWidget>

namespace jwrite {

namespace Ui {
struct TitleBar;
} // namespace Ui

struct TitleBarPrivate;

class TitleBar : public QWidget {
    Q_OBJECT

public:
    explicit TitleBar(QWidget *parent = nullptr);
    virtual ~TitleBar();

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

    QString get_title() const {
        return title_;
    }

    void set_title(const QString &title) {
        title_ = title;
        update();
    }

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    Ui::TitleBar *ui;
    QString       title_;
};

} // namespace jwrite
