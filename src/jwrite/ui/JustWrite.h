#pragma once

#include <jwrite/ui/TitleBar.h>
#include <jwrite/ui/EditPage.h>
#include <jwrite/ui/Gallery.h>
#include <jwrite/ui/FloatingDialog.h>
#include <jwrite/BookManager.h>
#include <QWidget>
#include <QStackedWidget>
#include <QStackedLayout>
#include <QWKWidgets/widgetwindowagent.h>
#include <optional>

namespace jwrite::ui {

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

signals:
    void pageChanged(PageType page);

public:
    void setTheme(Theme theme);
    void updateBookInfo(int index, const BookInfo &info);

protected:
    QString getLikelyAuthor() const;
    void    updateLikelyAuthor(const QString &author);

public:
    void toggleMaximize();

protected:
    void    setupUi();
    void    setupConnections();
    void    requestUpdateBookInfo(int index);
    QString requestImagePath(bool validate, QImage *out_image);
    void    requestBookAction(int index, Gallery::MenuAction action);
    void    requestStartEditBook(int index);
    void    requestRenameTocItem(const BookInfo &book_info, int vid, int cid);

    void showPopupLayer(QWidget *widget);
    void closePopupLayer();

    void switchToPage(PageType page);
    void closePage();

private:
    QMap<PageType, QWidget *>            page_map_;
    PageType                             current_page_;
    QMap<QString, AbstractBookManager *> books_;
    QString                              likely_author_;

    TitleBar               *ui_title_bar_;
    Gallery                *ui_gallery_;
    EditPage               *ui_edit_page_;
    FloatingDialog         *ui_popup_layer_;
    QStackedWidget         *ui_page_stack_;
    QStackedLayout         *ui_top_most_layout_;
    QWK::WidgetWindowAgent *ui_agent_;
};

} // namespace jwrite::ui
