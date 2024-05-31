#pragma once

#include <jwrite/ui/TitleBar.h>
#include <jwrite/ui/Toolbar.h>
#include <jwrite/ui/EditPage.h>
#include <jwrite/ui/Gallery.h>
#include <jwrite/BookManager.h>
#include <jwrite/GlobalCommand.h>
#include <widget-kit/OverlaySurface.h>
#include <widget-kit/Progress.h>
#include <QWKWidgets/widgetwindowagent.h>
#include <QWidget>
#include <QStackedWidget>
#include <QStackedLayout>
#include <QSystemTrayIcon>
#include <functional>
#include <optional>

namespace jwrite::ui {

class JustWrite : public QWidget {
    Q_OBJECT

public:
    enum class TocType {
        Volume,
        Chapter,
    };

    enum class PageType {
        Gallery,
        Edit,
    };

    enum class ExportType {
        PlainText,
        ePub,
    };

protected:
    enum ToolbarItemType {
        TI_Gallery,
        TI_Draft,
        TI_Favorites,
        TI_Trash,
        TI_Export,
        TI_Share,
        TI_Fullscreen,
        TI_ExitFullscreen,
        TI_Help,
        TI_Settings,
    };

signals:
    void on_trigger_shortcut(GlobalCommand shortcut);

protected:
    void request_create_new_book();
    void do_create_book(BookInfo &book_info);

    void request_remove_book(const QString &book_id);
    void do_remove_book(const QString &book_id);

    void request_open_book(const QString &book_id);
    void do_open_book(const QString &book_id);

    void request_close_opened_book();
    void do_close_book(const QString &book_id);

    void do_update_book_info(BookInfo &book_info);
    void request_update_book_info(const QString &book_id);

    void request_rename_toc_item(const QString &book_id, int toc, TocType type);
    void do_rename_toc_item(const QString &book_id, int toc, const QString &title);

    void request_export_book(const QString &book_id);
    bool do_export_book(const QString &book_id, const QString &path, ExportType type);
    bool do_export_book_as_plain_text(const QString &book_id, const QString &path);
    bool do_export_book_as_epub(const QString &book_id, const QString &path);

public:
    bool is_fullscreen_mode() const {
        return fullscreen_;
    }

    void set_fullscreen_mode(bool enable);
    void trigger_shortcut(GlobalCommand shortcut);

public slots:
    void handle_gallery_on_click(int index);
    void handle_gallery_on_menu_action(int index, Gallery::MenuAction action);
    void handle_gallery_on_load_book(const BookInfo &book_info);
    void handle_book_dir_on_rename_toc_item(const QString &book_id, int toc_id, TocType type);
    void handle_book_dir_on_rename_toc_item__adapter(const BookInfo &book_info, int vid, int cid);
    void handle_edit_page_on_export();
    void handle_on_page_change(PageType page);
    void handle_on_open_help();
    void handle_on_open_settings();
    void handle_on_theme_change();
    void handle_on_scheme_change(const ColorScheme &scheme);
    void handle_on_minimize();
    void handle_on_toggle_maximize();
    void handle_on_close();
    void handle_on_open_gallery();
    void handle_on_enter_fullscreen();
    void handle_on_exit_fullscreen();
    void handle_on_visiblity_change(bool visible);
    void handle_on_trigger_shortcut(GlobalCommand shortcut);

public:
    QString get_default_author() const;
    void    set_default_author(const QString &author, bool force);

    std::optional<GlobalCommand> try_match_shortcut(QKeyEvent *event) const {
        return command_manager_.match(event);
    }

    void wait(std::function<void()> job);

    static widgetkit::Progress::Builder get_wait_builder() {
        return widgetkit::Progress::Builder{};
    }

public:
    JustWrite();
    ~JustWrite();

signals:
    void pageChanged(PageType page);

public:
    void updateColorScheme(const ColorScheme &scheme);

protected:
    void setupUi();
    void setupConnections();
    void requestStartEditBook(int index);

    void requestInitFromLocalStorage();

    void initLocalStorage();
    void loadDataFromLocalStorage();
    void syncToLocalStorage();

    void switchToPage(PageType page);

    bool eventFilter(QObject *watched, QEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;

private:
    QMap<PageType, QWidget *>            page_map_;
    QMap<PageType, QSet<int>>            page_toolbar_mask_;
    PageType                             current_page_;
    QMap<QString, AbstractBookManager *> books_;
    QString                              likely_author_;
    GlobalCommandManager                 command_manager_;
    bool                                 fullscreen_;

    QSystemTrayIcon           *ui_tray_icon_;
    TitleBar                  *ui_title_bar_;
    Toolbar                   *ui_toolbar_;
    Gallery                   *ui_gallery_;
    EditPage                  *ui_edit_page_;
    widgetkit::OverlaySurface *ui_surface_;
    QStackedWidget            *ui_page_stack_;
    QWK::WidgetWindowAgent    *ui_agent_;
};

} // namespace jwrite::ui
