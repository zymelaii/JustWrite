#pragma once

#include "Editor.h"
#include "StatusBar.h"
#include "FlatButton.h"
#include "TitleBar.h"
#include "TwoLevelTree.h"
#include "../GlobalCommand.h"
#include "../MessyInput.h"
#include "../VisualTextEditContext.h"
#include <QWidget>
#include <QTimer>
#include <QLabel>
#include <QAction>
#include <QWKWidgets/widgetwindowagent.h>

namespace jwrite::Ui {

struct JustWritePrivate;

class JustWrite : public QWidget {
    Q_OBJECT

protected:
    enum class ExportType {
        PlainText,
        ePub,
    };

public:
    explicit JustWrite(QWidget *parent = nullptr);
    virtual ~JustWrite();

public slots:
    int  addVolume(int index, const QString &title);
    int  addChapter(int volume_index, const QString &title);
    void openChapter(int cid);

    void renameBookDirItem(int id, const QString &title);
    void exportToLocal(const QString &path, ExportType type);

protected:
    void setupUi();
    void popupBookDirMenu(QPoint pos, TwoLevelTree::ItemInfo item_info);
    void requestExportToLocal();

    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    jwrite::GlobalCommandManager              command_manager_;
    jwrite::MessyInputWorker                 *messy_input_;
    QTimer                                    sec_timer_;
    int                                       current_cid_;
    QMap<int, QString>                        chapters_;
    QMap<int, VisualTextEditContext::TextLoc> chapter_locs_;
    QString                                   book_name_;
    QString                                   author_;

    jwrite::Ui::TitleBar     *ui_title_bar;
    jwrite::Ui::Editor       *ui_editor;
    jwrite::Ui::StatusBar    *ui_status_bar;
    jwrite::Ui::FlatButton   *ui_new_volume;
    jwrite::Ui::FlatButton   *ui_new_chapter;
    jwrite::Ui::FlatButton   *ui_export_to_local;
    jwrite::Ui::TwoLevelTree *ui_book_dir;
    QLabel                   *ui_total_words;
    QLabel                   *ui_datetime;
    QWidget                  *ui_sidebar;
    QWK::WidgetWindowAgent   *ui_agent;
};

} // namespace jwrite::Ui
