#pragma once

#include <jwrite/ui/TitleBar.h>
#include <jwrite/ui/EditPage.h>
#include <jwrite/ui/Gallery.h>
#include <jwrite/ui/FloatingDialog.h>
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
    JustWrite();
    ~JustWrite();

public:
    void setTheme(Theme theme);

protected:
    void setupUi();
    void setupConnections();

    void showPopupLayer(QWidget *widget);
    void closePopupLayer();

    void switchToPage(PageType page);

private:
    QMap<PageType, QWidget *> page_map_;

    jwrite::Ui::TitleBar       *ui_title_bar_;
    jwrite::Ui::Gallery        *ui_gallery_;
    jwrite::Ui::EditPage       *ui_edit_page_;
    jwrite::ui::FloatingDialog *ui_popup_layer_;
    QStackedWidget             *ui_page_stack_;
    QStackedLayout             *ui_top_most_layout_;
    QWK::WidgetWindowAgent     *ui_agent_;
};

} // namespace jwrite::Ui
