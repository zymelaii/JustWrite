#pragma once

#include "TitleBar.h"
#include "EditPage.h"
#include "Gallery.h"
#include <QWidget>
#include <QStackedWidget>
#include <QStackedLayout>
#include <QWKWidgets/widgetwindowagent.h>

namespace jwrite::Ui {

class JustWrite : public QWidget {
    Q_OBJECT

protected:
    enum class PageType {
        Edit,
        Gallery,
    };

    enum class Theme {
        Light,
        Dark,
    };

public:
    explicit JustWrite(QWidget *parent = nullptr);
    ~JustWrite();

public:
    void setTheme(Theme theme);

protected:
    void setupUi();

    void switchToPage(PageType page);

private:
    QMap<PageType, QWidget *> page_map_;

    jwrite::Ui::TitleBar   *ui_title_bar;
    QStackedWidget         *ui_page_stack;
    jwrite::Ui::Gallery    *ui_gallery;
    jwrite::Ui::EditPage   *ui_edit_page;
    QStackedLayout         *ui_top_most_layout;
    QWidget                *ui_popup_layer;
    QWK::WidgetWindowAgent *ui_agent;
};

} // namespace jwrite::Ui
