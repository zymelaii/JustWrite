#pragma once

#include <QWidget>

namespace jwrite::Ui {

class Gallery : public QWidget {
    Q_OBJECT

public:
    explicit Gallery(QWidget *parent = nullptr);
    ~Gallery();

private:
    void setupUi();

    void paintEvent(QPaintEvent *event) override;
};

} // namespace jwrite::Ui
