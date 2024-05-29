#pragma once

#include <jwrite/ColorScheme.h>
#include <QWidget>
#include <QVBoxLayout>

namespace jwrite::ui {

class Toolbar : public QWidget {
    Q_OBJECT

public:
    void update_color_scheme(const ColorScheme &scheme);
    void reload_toolbar_icons();
    void add_item(const QString &tip, const QString &icon_name, QAction *action, bool bottom_side);

public:
    explicit Toolbar(QWidget *parent = nullptr);

private:
    void init();

private:
    int          ui_total_bottom_side_;
    QVBoxLayout *ui_layout_;
};

} // namespace jwrite::ui
