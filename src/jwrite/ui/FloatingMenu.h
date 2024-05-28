#pragma once

#include <QWidget>
#include <QAction>

namespace jwrite::ui {

class FloatingMenu : public QWidget {
    Q_OBJECT

public:
    int  get_icon_size() const;
    int  get_item_size() const;
    void set_background_color(const QColor& color);
    void set_border_color(const QColor& color);
    void set_hover_color(const QColor& color);
    void update_geometry();
    void reload_icon();
    void set_expanded(bool expanded);
    void add_menu_item(const QString& icon_name, QAction* action);
    int  get_index_of_menu_item(const QPoint& pos);

public:
    explicit FloatingMenu(QWidget* parent);
    ~FloatingMenu() override;

public:
    QSize minimumSizeHint() const override;
    QSize sizeHint() const override;

protected:
    void setupUi();

    bool eventFilter(QObject* watched, QEvent* event) override;
    bool event(QEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;

private:
    struct MenuItem {
        QString  icon_name;
        QAction* action;
    };

    QList<MenuItem> menu_items_;

    QColor         ui_background_color_;
    QColor         ui_border_color_;
    QColor         ui_hover_color_;
    bool           ui_expanded_;
    QPixmap        ui_menu_icon_;
    QList<QPixmap> ui_menu_items_;
    int            ui_hover_on_;
};

} // namespace jwrite::ui
