#pragma once

#include <jwrite/ui/Label.h>

namespace jwrite::ui {

class ColorSelectorButton : public Label {
    Q_OBJECT

public:
    explicit ColorSelectorButton(QWidget *parent = nullptr);
    ~ColorSelectorButton();

signals:
    void colorChanged(QColor color);

public:
    void   setColor(QColor &color);
    QColor color();

protected:
    void paintEvent(QPaintEvent *e);

private slots:
    void showColorSelector();

private:
    QColor color_;
};

} // namespace jwrite::ui
