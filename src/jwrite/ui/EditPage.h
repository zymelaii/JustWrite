#pragma once

#include "Editor.h"
#include "StatusBar.h"
#include "FlatButton.h"
#include "TwoLevelTree.h"
#include "../MessyInput.h"
#include "../VisualTextEditContext.h"
#include "../GlobalCommand.h"
#include "../ColorTheme.h"
#include <QWidget>
#include <QTimer>
#include <QLabel>
#include <QAction>

namespace jwrite::Ui {

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

public:
    void updateColorTheme(const ColorTheme &color_theme);

    void setCurrentBookInfo(const QString &name, const QString &author);

    int  addVolume(int index, const QString &title);
    int  addChapter(int volume_index, const QString &title);
    void openChapter(int cid);

    void renameBookDirItem(int id, const QString &title);
    void exportToLocal(const QString &path, ExportType type);

protected:
    void setupUi();
    void setupConnections();

    void popupBookDirMenu(QPoint pos, TwoLevelTree::ItemInfo item_info);
    void requestExportToLocal();
    void updateWordsCount(const QString &text);
    void createAndOpenNewChapter();

    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    jwrite::GlobalCommandManager              command_manager_;
    jwrite::MessyInputWorker                 *messy_input_;
    QTimer                                    sec_timer_;
    int                                       current_cid_;
    QMap<int, QString>                        chapters_;
    QMap<int, VisualTextEditContext::TextLoc> chapter_locs_;
    int                                       chap_words_;
    int                                       total_words_;
    QString                                   book_name_;
    QString                                   author_;

    jwrite::Ui::Editor       *ui_editor;
    jwrite::Ui::StatusBar    *ui_status_bar;
    jwrite::Ui::FlatButton   *ui_new_volume;
    jwrite::Ui::FlatButton   *ui_new_chapter;
    jwrite::Ui::FlatButton   *ui_export_to_local;
    jwrite::Ui::TwoLevelTree *ui_book_dir;
    QLabel                   *ui_total_words;
    QLabel                   *ui_datetime;
    QWidget                  *ui_sidebar;
    QMap<QString, QWidget *>  ui_named_widgets;
};

} // namespace jwrite::Ui
