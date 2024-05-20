#include <jwrite/ui/EditPage.h>
#include <jwrite/TextInputCommand.h>
#include <jwrite/ProfileUtils.h>
#include <jwrite/epub/EpubBuilder.h>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QSplitter>
#include <QDateTime>
#include <QFrame>
#include <QPainter>
#include <QMenu>
#include <QFileDialog>
#include <QMessageBox>

namespace jwrite::Ui {

class BookDirItemRenderProxy
    : public TwoLevelTree::ItemRenderProxy
    , public QObject {
public:
    explicit BookDirItemRenderProxy(QObject *parent)
        : QObject(parent) {}

    void render(QPainter *p, const QRect &clip_bb, const TwoLevelTree::ItemInfo &item_info) {
        const auto format =
            item_info.is_top_item ? QStringLiteral("第 %1 卷 %2") : QStringLiteral("第 %1 章 %2");
        const auto title = format.arg(item_info.level_index + 1).arg(item_info.value);
        const auto flags = Qt::AlignLeft | Qt::AlignVCenter;
        p->drawText(clip_bb, flags, title);
    }
};

EditPage::EditPage(QWidget *parent)
    : QWidget(parent)
    , current_cid_{-1}
    , chap_words_{0}
    , total_words_{0}
    , messy_input_{new MessyInputWorker(this)} {
    setupUi();
    setupConnections();

    command_manager_.load_default();

    ui_editor->installEventFilter(this);

    sec_timer_.setInterval(1000);
    sec_timer_.setSingleShot(false);
    sec_timer_.start();
}

EditPage::~EditPage() {
    jwrite_profiler_dump(QStringLiteral("jwrite-profiler.%1.log")
                             .arg(QDateTime::currentDateTime().toString("yyyyMMddHHmmss")));

    messy_input_->kill();
    delete messy_input_;
}

void EditPage::updateColorTheme(const ColorTheme &color_theme) {
    auto pal = palette();
    pal.setColor(QPalette::Window, color_theme.Window);
    pal.setColor(QPalette::WindowText, color_theme.WindowText);
    pal.setColor(QPalette::Base, color_theme.TextBase);
    pal.setColor(QPalette::Text, color_theme.Text);
    pal.setColor(QPalette::Highlight, color_theme.SelectedText);
    pal.setColor(QPalette::Button, color_theme.Window);
    pal.setColor(QPalette::ButtonText, color_theme.WindowText);
    setPalette(pal);

    if (auto w = ui_named_widgets.value("sidebar.splitter")) {
        auto pal = w->palette();
        pal.setColor(QPalette::Window, color_theme.TextBase);
        w->setPalette(pal);
    }

    if (auto w = ui_sidebar) {
        auto pal = w->palette();
        pal.setColor(QPalette::Window, color_theme.Panel);
        w->setPalette(pal);
    }

    if (auto w = ui_editor) {
        auto pal = w->palette();
        pal.setColor(QPalette::Window, color_theme.TextBase);
        pal.setColor(QPalette::HighlightedText, color_theme.SelectedItem);
        w->setPalette(pal);
    }

    if (auto w = ui_named_widgets.value("sidebar.button-line")) {
        auto pal = w->palette();
        pal.setColor(QPalette::Base, color_theme.Panel);
        pal.setColor(QPalette::Button, color_theme.Hover);
        w->setPalette(pal);
    }

    if (auto w = ui_named_widgets.value("sidebar.sep-line")) {
        auto pal = w->palette();
        pal.setColor(QPalette::WindowText, color_theme.Border);
        w->setPalette(pal);
    }

    if (auto w = ui_book_dir) {
        auto pal = w->palette();
        pal.setColor(QPalette::Highlight, color_theme.SelectedItem);
        pal.setColor(QPalette::HighlightedText, color_theme.Hover);
        w->setPalette(pal);
    }
}

void EditPage::setCurrentBookInfo(const QString &name, const QString &author) {
    book_name_ = name;
    author_    = author;
}

int EditPage::addVolume(int index, const QString &title) {
    Q_ASSERT(index >= 0 && index <= ui_book_dir->totalTopItems());
    return ui_book_dir->addTopItem(index, title);
}

int EditPage::addChapter(int volume_index, const QString &title) {
    Q_ASSERT(volume_index >= 0 && volume_index < ui_book_dir->totalTopItems());
    const auto vid = ui_book_dir->topItemAt(volume_index);
    Q_ASSERT(vid != -1);
    const int chap_index = ui_book_dir->totalSubItemsUnderTopItem(vid);
    return ui_book_dir->addSubItem(vid, chap_index, title);
}

void EditPage::openChapter(int cid) {
    if (messy_input_->is_running()) { return; }

    if (cid == current_cid_) { return; }

    jwrite_profiler_start(SwitchChapter);

    QString  tmp{};
    QString &text = chapters_.contains(cid) ? chapters_[cid] : tmp;
    chap_words_   = text.length();

    if (current_cid_ != -1) {
        const auto loc = ui_editor->currentTextLoc();
        if (loc.block_index != -1) { chapter_locs_[current_cid_] = loc; }
    }

    ui_editor->reset(text, true);
    chapters_[current_cid_] = text;
    current_cid_            = cid;

    if (chapter_locs_.contains(cid)) {
        const auto loc = chapter_locs_[cid];
        ui_editor->setCursorToTextLoc(loc);
    }

    jwrite_profiler_record(SwitchChapter);
}

void EditPage::renameBookDirItem(int id, const QString &title) {
    ui_book_dir->setItemValue(id, title);
}

void EditPage::exportToLocal(const QString &path, ExportType type) {
    switch (type) {
        case ExportType::PlainText: {
            QFile file(path);
            if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QMessageBox::warning(this, "导出失败", "无法打开文件：" + path);
                return;
            }

            QTextStream out(&file);

            const int total_volumes = ui_book_dir->totalTopItems();
            int       chap_index    = 0;
            for (int i = 0; i < total_volumes; ++i) {
                const int vid = ui_book_dir->topItemAt(i);
                out << QStringLiteral("【第 %1 卷 %2】\n\n")
                           .arg(i + 1)
                           .arg(ui_book_dir->itemValue(vid));
                const auto chaps = ui_book_dir->getSubItems(vid);
                for (const auto cid : chaps) {
                    const auto content =
                        current_cid_ == cid ? ui_editor->text() : chapters_.value(cid, "");
                    out << QStringLiteral("第 %1 章 %2\n")
                               .arg(++chap_index)
                               .arg(ui_book_dir->itemValue(cid))
                        << content << "\n\n";
                }
            }
        } break;
        case ExportType::ePub: {
            jwrite::epub::EpubBuilder builder(path);
            for (int i = 0; i < ui_book_dir->totalTopItems(); ++i) {
                const int vid = ui_book_dir->topItemAt(i);
                builder.with_volume(
                    QString("第 %1 卷 %2").arg(i + 1).arg(ui_book_dir->itemValue(vid)),
                    ui_book_dir->totalSubItemsUnderTopItem(vid));
            }
            int global_chap_index = 0;
            builder.with_name(book_name_.isEmpty() ? "未命名" : book_name_)
                .with_author(author_.isEmpty() ? "佚名" : author_)
                .feed([this, &global_chap_index](
                          int      vol_index,
                          int      chap_index,
                          QString &out_chap_title,
                          QString &out_content) {
                    const int vid  = ui_book_dir->topItemAt(vol_index);
                    const int cid  = ui_book_dir->getSubItem(vid, chap_index);
                    out_chap_title = QString("第 %1 章 %2")
                                         .arg(++global_chap_index)
                                         .arg(ui_book_dir->itemValue(cid));
                    out_content =
                        current_cid_ == cid ? ui_editor->text() : chapters_.value(cid, "");
                })
                .build();
        } break;
    }
}

void EditPage::setupUi() {
    auto layout         = new QVBoxLayout(this);
    auto content        = new QWidget;
    auto layout_content = new QHBoxLayout(content);
    auto splitter       = new QSplitter;
    ui_named_widgets.insert("sidebar.splitter", splitter);

    ui_editor     = new jwrite::Ui::Editor;
    ui_status_bar = new jwrite::Ui::StatusBar;

    ui_sidebar         = new QWidget;
    ui_new_volume      = new FlatButton;
    ui_new_chapter     = new FlatButton;
    ui_export_to_local = new FlatButton;
    ui_book_dir        = new jwrite::Ui::TwoLevelTree;

    auto btn_line        = new QWidget;
    auto btn_line_layout = new QHBoxLayout(btn_line);
    btn_line_layout->setContentsMargins({});
    btn_line_layout->addStretch();
    btn_line_layout->addWidget(ui_new_volume);
    btn_line_layout->addStretch();
    btn_line_layout->addWidget(ui_new_chapter);
    btn_line_layout->addStretch();
    btn_line_layout->addWidget(ui_export_to_local);
    btn_line_layout->addStretch();
    ui_named_widgets.insert("sidebar.button-line", btn_line);

    auto sep_line = new QFrame;
    sep_line->setFrameShape(QFrame::HLine);
    ui_named_widgets.insert("sidebar.sep-line", sep_line);

    auto sidebar_layout = new QVBoxLayout(ui_sidebar);
    sidebar_layout->setContentsMargins(0, 10, 0, 10);
    sidebar_layout->addWidget(btn_line);
    sidebar_layout->addWidget(sep_line);
    sidebar_layout->addWidget(ui_book_dir);

    ui_sidebar->setAutoFillBackground(true);
    ui_sidebar->setMinimumWidth(200);

    splitter->addWidget(ui_sidebar);
    splitter->addWidget(ui_editor);

    layout->addWidget(content);
    layout->addWidget(ui_status_bar);
    layout->setContentsMargins({});
    layout->setSpacing(0);

    layout_content->addWidget(splitter);
    layout_content->setContentsMargins({});

    splitter->setOrientation(Qt::Horizontal);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setCollapsible(0, false);
    splitter->setCollapsible(1, false);

    ui_new_volume->setText("新建卷");
    ui_new_chapter->setText("新建章");
    ui_export_to_local->setText("导出");

    ui_total_words = ui_status_bar->addItem("全书共 0 字 本章 0 字", false);
    ui_datetime    = ui_status_bar->addItem("0000-00-00", true);

    auto font = this->font();
    font.setPointSize(10);
    ui_sidebar->setFont(font);

    content->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

    //! install book dir item render proxy
    ui_book_dir->setItemRenderProxy(new BookDirItemRenderProxy(this));

    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    setAutoFillBackground(true);
}

void EditPage::setupConnections() {
    connect(ui_editor, &Editor::activated, this, [this] {
        Q_ASSERT(ui_book_dir->totalTopItems() == 0);
        const int vid = addVolume(0, "默认卷");
        const int cid = addChapter(0, "");
        current_cid_  = cid;
        ui_book_dir->setSubItemSelected(vid, cid);
    });
    connect(ui_editor, &Editor::textChanged, this, &EditPage::updateWordsCount);
    connect(ui_book_dir, &TwoLevelTree::subItemSelected, this, [this](int vid, int cid) {
        openChapter(cid);
    });
    connect(ui_new_volume, &FlatButton::pressed, this, [this] {
        addVolume(ui_book_dir->totalTopItems(), "");
    });
    connect(ui_new_chapter, &FlatButton::pressed, this, [this] {
        createAndOpenNewChapter();
    });
    connect(ui_export_to_local, &FlatButton::pressed, this, &EditPage::requestExportToLocal);
    connect(&sec_timer_, &QTimer::timeout, this, [this] {
        ui_datetime->setText(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"));
    });
    connect(ui_editor, &Editor::focusLost, this, [this] {
        messy_input_->kill();
    });
    connect(ui_book_dir, &TwoLevelTree::contextMenuRequested, this, &EditPage::popupBookDirMenu);
}

void EditPage::popupBookDirMenu(QPoint pos, TwoLevelTree::ItemInfo item_info) {
    //! TODO: menu style & popup input to edit the new title
}

void EditPage::requestExportToLocal() {
    const auto caption      = "导出到本地";
    const auto default_name = book_name_.isEmpty() ? "未命名书籍" : book_name_;

    QMap<QString, ExportType> types;
    types[QStringLiteral("文本文件 (*.txt)")]     = ExportType::PlainText;
    types[QStringLiteral("EPUB 电子书 (*.epub)")] = ExportType::ePub;

    QString selected_type;
    auto    path = QFileDialog::getSaveFileName(
        this, caption, default_name, types.keys().join("\n"), &selected_type);

    if (path.isEmpty()) { return; }

    const auto type = types[selected_type];

    if (QFileInfo(path).suffix().isEmpty()) {
        switch (type) {
            case ExportType::PlainText: {
                path.append(".txt");
            } break;
            case ExportType::ePub: {
                path.append(".epub");
            } break;
        }
    }

    exportToLocal(path, type);
}

void EditPage::updateWordsCount(const QString &text) {
    const int words_diff  = text.length() - chap_words_;
    chap_words_          += words_diff;
    total_words_         += words_diff;
    ui_total_words->setText(QString("全书共 %1 字 本章 %2 字").arg(total_words_).arg(chap_words_));
}

void EditPage::createAndOpenNewChapter() {
    const int volume_index = ui_book_dir->totalTopItems() - 1;
    const int cid          = addChapter(volume_index, "");
    openChapter(cid);
    ui_book_dir->setSubItemSelected(ui_book_dir->topItemAt(volume_index), cid);
}

bool EditPage::eventFilter(QObject *watched, QEvent *event) {
    if (event->type() == QEvent::KeyPress) {
        auto e = static_cast<QKeyEvent *>(event);

        //! NOTE: block special keys in messy input mode to avoid unexpceted behavior
        //! ATTENTION: this can not block global shortcut keys
        if (const auto key = QKeyCombination::fromCombined(e->key() | e->modifiers());
            !TextInputCommandManager::is_printable_char(key) && messy_input_->is_running()) {
            return true;
        }

        if (auto opt = command_manager_.match(e)) {
            const auto action = *opt;
            qDebug() << "COMMAND" << magic_enum::enum_name(action).data();

            switch (action) {
                case GlobalCommand::ToggleSidebar: {
                    ui_sidebar->setVisible(!ui_sidebar->isVisible());
                } break;
                case GlobalCommand::ToggleSoftCenterMode: {
                    ui_editor->setSoftCenterMode(!ui_editor->softCenterMode());
                } break;
                case GlobalCommand::CreateNewChapter: {
                    createAndOpenNewChapter();
                } break;
                case GlobalCommand::DEV_EnableMessyInput: {
                    ui_editor->setFocus();
                    messy_input_->start();
                } break;
            }
            return true;
        }
    }

    if (event->type() == QEvent::MouseButtonDblClick) {
        auto e = static_cast<QMouseEvent *>(event);
        if (e->buttons() == Qt::RightButton) {
            messy_input_->kill();
            return false;
        }
    }

    return false;
}

} // namespace jwrite::Ui