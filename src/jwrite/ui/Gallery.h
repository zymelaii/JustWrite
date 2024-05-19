#pragma once

#include <jwrite/ColorTheme.h>
#include <QWidget>

namespace jwrite::Ui {

class Gallery : public QWidget {
    Q_OBJECT

public:
    explicit Gallery(QWidget *parent = nullptr);
    ~Gallery();

public:
    void updateColorTheme(const ColorTheme &color_theme);

private:
    void setupUi();

    void paintEvent(QPaintEvent *event) override;

private:
};

} // namespace jwrite::Ui
