#pragma once

#include <jwrite/ui/Editor.h>
#include <jwrite/ui/StatusBar.h>
#include <jwrite/ui/FlatButton.h>
#include <jwrite/ui/TwoLevelTree.h>
#include <jwrite/MessyInput.h>
#include <jwrite/VisualTextEditContext.h>
#include <jwrite/GlobalCommand.h>
#include <jwrite/ColorTheme.h>
#include <jwrite/BookManager.h>
#include <QWidget>
#include <QTimer>
#include <QLabel>
#include <QAction>

namespace jwrite::ui {

class EditPage : public QWidget {
    Q_OBJECT

protected:
    enum class ExportType {
        PlainText,
        ePub,
    };

public:
    explicit EditPage(QWidget *parent = nullptr);
    virtual ~EditPage();

signals:
    void renameTocItemRequested(const BookInfo &book_info, int vid, int cid);

public:
    void updateColorTheme(const ColorTheme &color_theme);

    AbstractBookManager *resetBookSource(AbstractBookManager *book_manager);
    AbstractBookManager *takeBookSource();

    const AbstractBookManager &bookSource() const {
        Q_ASSERT(book_manager_);
        return *book_manager_;
    }

    int  addVolume(int index, const QString &title);
    int  addChapter(int volume_index, const QString &title);
    void openChapter(int cid);
    void syncAndClearEditor();

    void renameBookDirItem(int id, const QString &title);
    void exportToLocal(const QString &path, ExportType type);

protected slots:
    void updateCurrentDateTime();

protected:
    void setupUi();
    void setupConnections();

    void popupBookDirMenu(QPoint pos, TwoLevelTree::ItemInfo item_info);
    void requestExportToLocal();
    void updateWordsCount(const QString &text);
    void flushWordsCount();
    void syncWordsStatus();
    void createAndOpenNewChapter();
    void requestRenameTocItem();

    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    GlobalCommandManager                      command_manager_;
    MessyInputWorker                         *messy_input_;
    AbstractBookManager                      *book_manager_;
    QTimer                                    sec_timer_;
    int                                       current_cid_;
    QMap<int, VisualTextEditContext::TextLoc> chapter_locs_;
    int                                       chap_words_;
    int                                       total_words_;

    Editor                  *ui_editor;
    StatusBar               *ui_status_bar;
    FlatButton              *ui_new_volume;
    FlatButton              *ui_new_chapter;
    FlatButton              *ui_export_to_local;
    TwoLevelTree            *ui_book_dir;
    QLabel                  *ui_total_words;
    QLabel                  *ui_datetime;
    QWidget                 *ui_sidebar;
    QMap<QString, QWidget *> ui_named_widgets;
};

} // namespace jwrite::ui
