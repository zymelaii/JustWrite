#pragma once

#include <QWidget>

namespace jwrite::Ui {

class TitleBar : public QWidget {
    Q_OBJECT

public:
    explicit TitleBar(QWidget *parent = nullptr);
    virtual ~TitleBar();

public:
    QString title() const;
    void    setTitle(const QString &title);

public:
    QSize minimumSizeHint() const override;
    QSize sizeHint() const override;

protected:
    void setupUi();

    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    QString title_;
};

} // namespace jwrite::Ui
