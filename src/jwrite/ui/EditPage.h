#pragma once

#include <jwrite/ui/Editor.h>
#include <jwrite/ui/FloatingLabel.h>
#include <jwrite/ui/TwoLevelTree.h>
#include <jwrite/ui/FloatingMenu.h>
#include <jwrite/ColorScheme.h>
#include <jwrite/VisualTextEditContext.h>
#include <jwrite/WordCounter.h>
#include <jwrite/GlobalCommand.h>
#include <jwrite/BookManager.h>
#include <widget-kit/FlatButton.h>
#include <QWidget>
#include <QLabel>
#include <memory>

namespace jwrite::ui {

class EditPage : public QWidget {
    Q_OBJECT

signals:
    void on_request_rename_toc_item(const BookInfo &book_info, int vid, int cid);
    void on_request_quit_edit();
    void on_request_open_settings();

public:
    void request_rename_toc_item(int vid, int cid);

    void request_invalidate_wcstate();
    void request_sync_wcstate();
    void do_update_wcstate(const QString &text, bool text_changed);
    void do_flush_wcstate();

    void do_open_chapter(int cid);

public:
    static QString get_friendly_word_count(int count);

    void reset_word_counter(AbstractWordCounter *word_counter) {
        Q_ASSERT(word_counter);
        word_counter_.reset(word_counter);
        request_invalidate_wcstate();
        do_flush_wcstate();
        request_sync_wcstate();
    }

    void update_color_scheme(const ColorScheme &scheme);

    QString get_book_id_of_source() const;
    void    drop_source_ref();
    bool    reset_source(AbstractBookManager *book_manager);

    int  add_volume(int index, const QString &title);
    int  add_chapter(int volume_index, const QString &title);
    void sync_chapter_from_editor();
    void focus_editor();

    void rename_toc_item(int id, const QString &title);
    void create_and_open_chapter(int vid);

    void toggle_sidebar() {
        ui_sidebar_->setVisible(!ui_sidebar_->isVisible());
    }

    void toggle_soft_center_mode() {
        ui_editor_->setSoftCenterMode(!ui_editor_->softCenterMode());
    }

    Editor *editor() {
        return ui_editor_;
    }

    TwoLevelTree *book_dir() {
        return ui_book_dir_;
    }

public:
    void handle_editor_on_activate();
    void handle_editor_on_text_change(const QString &text);
    void handle_editor_on_focus_lost(VisualTextEditContext::TextLoc last_loc);
    void handle_book_dir_on_select_item(bool is_top_item, int top_item_id, int sub_item_id);
    void handle_book_dir_on_double_click_item(bool is_top_item, int top_item_id, int sub_item_id);
    void handle_book_dir_on_open_menu(QPoint pos, TwoLevelTree::ItemInfo item_info);
    void handle_on_create_volume();
    void handle_on_create_chapter();
    void handle_on_rename_selected_toc_item();

public:
    explicit EditPage(QWidget *parent = nullptr);

protected:
    void init();

private:
    std::unique_ptr<AbstractWordCounter>      word_counter_;
    AbstractBookManager                      *book_manager_;
    int                                       current_cid_;
    QMap<int, VisualTextEditContext::TextLoc> chapter_locs_;
    int                                       chap_words_;
    int                                       total_words_;
    VisualTextEditContext::TextLoc            last_loc_;

    Editor                  *ui_editor_;
    widgetkit::FlatButton   *ui_new_volume_;
    widgetkit::FlatButton   *ui_new_chapter_;
    TwoLevelTree            *ui_book_dir_;
    FloatingLabel           *ui_word_count_;
    QWidget                 *ui_sidebar_;
    QMap<QString, QWidget *> ui_named_widgets_;
    FloatingMenu            *ui_menu_;
};

} // namespace jwrite::ui
