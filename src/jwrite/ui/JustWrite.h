#pragma once

#include <jwrite/ui/TitleBar.h>
#include <jwrite/ui/EditPage.h>
#include <jwrite/ui/Gallery.h>
#include <jwrite/BookManager.h>
#include <jwrite/GlobalCommand.h>
#include <widget-kit/OverlaySurface.h>
#include <QWidget>
#include <QStackedWidget>
#include <QStackedLayout>
#include <QSystemTrayIcon>
#include <functional>
#include <QWKWidgets/widgetwindowagent.h>

namespace jwrite::ui {

class JustWrite : public QWidget {
    Q_OBJECT

protected:
    enum class PageType {
        Edit,
        Gallery,
    };

public:
    JustWrite();
    ~JustWrite();

signals:
    void pageChanged(PageType page);

public:
    void wait(std::function<void()> job);

    void updateColorScheme(const ColorScheme &scheme);
    void updateBookInfo(int index, const BookInfo &info);

protected:
    QString getLikelyAuthor() const;
    void    updateLikelyAuthor(const QString &author);

public:
    void toggleMaximize();

protected:
    void setupUi();
    void setupConnections();
    void requestUpdateBookInfo(int index);
    void requestBookAction(int index, Gallery::MenuAction action);
    void requestStartEditBook(int index);
    void requestRenameTocItem(const BookInfo &book_info, int vid, int cid);

    void requestInitFromLocalStorage();
    void requestQuitApp();

    void initLocalStorage();
    void loadDataFromLocalStorage();
    void syncToLocalStorage();

    void switchToPage(PageType page);
    void closePage();

    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    QMap<PageType, QWidget *>            page_map_;
    PageType                             current_page_;
    QMap<QString, AbstractBookManager *> books_;
    QString                              likely_author_;
    GlobalCommandManager                 command_manager_;
    QSystemTrayIcon                     *tray_icon_;
    TitleBar                            *ui_title_bar_;
    Gallery                             *ui_gallery_;
    EditPage                            *ui_edit_page_;
    widgetkit::OverlaySurface           *ui_surface_;
    QStackedWidget                      *ui_page_stack_;
    QWK::WidgetWindowAgent              *ui_agent_;
};

} // namespace jwrite::ui
