#pragma once

#include <jwrite/ui/Editor.h>
#include <jwrite/ui/FloatingLabel.h>
#include <jwrite/ui/StatusBar.h>
#include <jwrite/ui/TwoLevelTree.h>
#include <jwrite/ui/FloatingMenu.h>
#include <jwrite/MessyInput.h>
#include <jwrite/ColorScheme.h>
#include <jwrite/VisualTextEditContext.h>
#include <jwrite/WordCounter.h>
#include <jwrite/GlobalCommand.h>
#include <jwrite/BookManager.h>
#include <widget-kit/FlatButton.h>
#include <QWidget>
#include <QTimer>
#include <QLabel>

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
    ~EditPage() override;

signals:
    void renameTocItemRequested(const BookInfo &book_info, int vid, int cid);
    void quitEditRequested();
    void openSettingsRequested();

public:
    void updateColorScheme(const ColorScheme &scheme);

    bool                 resetBookSource(AbstractBookManager *book_manager);
    AbstractBookManager *takeBookSource();

    const AbstractBookManager &bookSource() const {
        Q_ASSERT(book_manager_);
        return *book_manager_;
    }

    int  addVolume(int index, const QString &title);
    int  addChapter(int volume_index, const QString &title);
    void openChapter(int cid);
    void syncAndClearEditor();
    void focusOnEditor();

    void renameBookDirItem(int id, const QString &title);

    void resetWordsCount();
    void updateWordsCount(const QString &text, bool text_changed);
    void flushWordsCount();
    void syncWordsStatus();

    Editor *editor() {
        return ui_editor_;
    }

    TwoLevelTree *book_dir() {
        return ui_book_dir_;
    }

protected slots:
    void updateCurrentDateTime();

protected:
    void setupUi();
    void setupConnections();

    void popupBookDirMenu(QPoint pos, TwoLevelTree::ItemInfo item_info);
    void createAndOpenNewChapter(int vid);
    void createAndOpenNewChapterUnderActiveVolume();
    void requestRenameCurrentTocItem();
    void requestRenameTocItem(int vid, int cid);
    bool handleShortcuts(QKeyEvent *event);

    void keyPressEvent(QKeyEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    GlobalCommandManager                      command_manager_;
    MessyInputWorker                         *messy_input_;
    AbstractWordCounter                      *word_counter_;
    AbstractBookManager                      *book_manager_;
    QTimer                                    sec_timer_;
    int                                       current_cid_;
    QMap<int, VisualTextEditContext::TextLoc> chapter_locs_;
    int                                       chap_words_;
    int                                       total_words_;
    VisualTextEditContext::TextLoc            last_loc_;

    Editor                  *ui_editor_;
    StatusBar               *ui_status_bar_;
    widgetkit::FlatButton   *ui_new_volume_;
    widgetkit::FlatButton   *ui_new_chapter_;
    TwoLevelTree            *ui_book_dir_;
    FloatingLabel           *ui_word_count_;
    QLabel                  *ui_total_words_;
    QLabel                  *ui_datetime_;
    QWidget                 *ui_sidebar_;
    QMap<QString, QWidget *> ui_named_widgets_;
    FloatingMenu            *ui_menu_;
};

} // namespace jwrite::ui
