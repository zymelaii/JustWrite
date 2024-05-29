#pragma once

#include <QLabel>

namespace widgetkit {

class ToolTipLabel : protected QWidget {
private:
    ToolTipLabel();

public:
    void show_tip(const QPoint &pos, const QString &text);
    void hide_tip();

    static ToolTipLabel &get_instance();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    QRect    rect_;
    QWidget *widget_;
    QString  text_;
    QPoint   pos_;
};

class ToolTip {
public:
    ToolTip() = delete;

    static void show_text(const QPoint &pos, const QString &text);

    static bool is_visible();
    static void set_font(const QFont &font);
    static void set_font_size(int size);
    static void set_background_color(const QColor &color);
    static void set_text_color(const QColor &color);
};

} // namespace widgetkit
