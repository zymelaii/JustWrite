#pragma once

#include <jwrite/ColorScheme.h>
#include <QWidget>
#include <QVBoxLayout>

namespace jwrite::ui {

class Toolbar : public QWidget {
    Q_OBJECT

public slots:
    void update_color_scheme(const ColorScheme &scheme);
    void set_icon_size(int size);
    void reload_toolbar_icons();

public:
    void add_item(const QString &tip, const QString &icon_name, QAction *action, bool bottom_side);
    void apply_mask(const QSet<int> &mask);

public:
    explicit Toolbar(QWidget *parent = nullptr);

private:
    void init();

private:
    int          ui_icon_size_;
    int          ui_margin_;
    int          ui_total_bottom_side_;
    QVBoxLayout *ui_layout_;
};

} // namespace jwrite::ui
