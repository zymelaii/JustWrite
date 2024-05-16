#include "JustWrite.h"
#include "../TextInputCommand.h"
#include "../ProfileUtils.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QSplitter>
#include <QDateTime>
#include <QFrame>
#include <QPainter>

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

JustWrite::JustWrite(QWidget *parent)
    : QWidget(parent)
    , current_cid_{-1}
    , messy_input_{new MessyInputWorker(this)} {
    command_manager_.load_default();
    sec_timer_.setInterval(1000);
    sec_timer_.setSingleShot(false);

    setupUi();

    ui_editor->installEventFilter(this);
    ui_editor->setFocus();

    connect(ui_editor, &Editor::requireEmptyChapter, this, &JustWrite::openEmptyChapter);
    connect(ui_editor, &Editor::textChanged, this, [this](const QString &text) {
        ui_total_words->setText(QString("全本共 %1 字").arg(text.length()));
    });
    connect(ui_book_dir, &TwoLevelTree::subItemSelected, this, [this](int vid) {
        openChapter(vid);
    });
    connect(ui_new_volume, &FlatButton::pressed, this, [this] {
        createNewVolume(-1, "");
    });
    connect(ui_new_chapter, &FlatButton::pressed, this, [this] {
        createAndOpenNewChapter(-1, "");
    });
    connect(&sec_timer_, &QTimer::timeout, this, [this] {
        ui_datetime->setText(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"));
    });
    connect(ui_editor, &Editor::focusLost, this, [this] {
        messy_input_->kill();
    });

    sec_timer_.start();
}

JustWrite::~JustWrite() {
    jwrite_profiler_dump(QStringLiteral("jwrite-profiler.%1.log")
                             .arg(QDateTime::currentDateTime().toString("yyyyMMddHHmmss")));

    messy_input_->kill();
    delete messy_input_;
}

void JustWrite::createNewVolume(int index, const QString &title) {
    if (index == -1) { index = ui_book_dir->totalTopItems(); }
    ui_book_dir->addTopItem(index, title);
}

void JustWrite::createAndOpenNewChapter(int volume_index, const QString &title) {
    if (volume_index == -1) { volume_index = ui_book_dir->totalTopItems() - 1; }
    const auto vid = ui_book_dir->topItemAt(volume_index);
    Q_ASSERT(vid != -1);
    const int chap_index = ui_book_dir->totalSubItemsUnderTopItem(vid);
    const int cid        = ui_book_dir->addSubItem(vid, chap_index, title);
    openChapter(cid);
}

void JustWrite::openEmptyChapter() {
    const auto vid = ui_book_dir->topItemAt(0);
    if (vid == -1) { createNewVolume(0, ""); }
    createAndOpenNewChapter(-1, "");
}

void JustWrite::openChapter(int cid) {
    if (messy_input_->is_running()) { return; }

    if (cid == current_cid_) { return; }

    jwrite_profiler_start(SwitchChapter);

    QString  tmp{};
    QString &text = chapters_.contains(cid) ? chapters_[cid] : tmp;

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

void JustWrite::setupUi() {
    auto layout         = new QVBoxLayout(this);
    auto content        = new QWidget;
    auto layout_content = new QHBoxLayout(content);
    auto splitter       = new QSplitter;
    ui_title_bar        = new jwrite::Ui::TitleBar;
    ui_editor           = new jwrite::Ui::Editor;
    ui_status_bar       = new jwrite::Ui::StatusBar;

    auto font = this->font();
    auto pal  = palette();

    //! setup left sidebar
    ui_sidebar     = new QWidget;
    ui_new_volume  = new FlatButton;
    ui_new_chapter = new FlatButton;
    ui_book_dir    = new jwrite::Ui::TwoLevelTree;

    auto btn_line        = new QWidget;
    auto btn_line_layout = new QHBoxLayout(btn_line);
    btn_line_layout->setContentsMargins({});
    btn_line_layout->addStretch();
    btn_line_layout->addWidget(ui_new_volume);
    btn_line_layout->addStretch();
    btn_line_layout->addWidget(ui_new_chapter);
    btn_line_layout->addStretch();

    auto sep_line = new QFrame;
    sep_line->setFrameShape(QFrame::HLine);

    auto sidebar_layout = new QVBoxLayout(ui_sidebar);
    sidebar_layout->setContentsMargins(0, 10, 0, 10);
    sidebar_layout->addWidget(btn_line);
    sidebar_layout->addWidget(sep_line);
    sidebar_layout->addWidget(ui_book_dir);

    font = this->font();
    font.setPointSize(10);
    ui_sidebar->setFont(font);

    pal = palette();
    pal.setColor(QPalette::WindowText, QColor(70, 70, 70));
    sep_line->setPalette(pal);

    ui_new_volume->setContentsMargins(10, 0, 10, 0);
    ui_new_volume->setText("新建卷");
    ui_new_volume->setTextAlignment(Qt::AlignCenter);
    ui_new_volume->setRadius(6);

    ui_new_chapter->setContentsMargins(10, 0, 10, 0);
    ui_new_chapter->setText("新建章");
    ui_new_chapter->setTextAlignment(Qt::AlignCenter);
    ui_new_chapter->setRadius(6);

    ui_sidebar->setAutoFillBackground(true);
    ui_sidebar->setMinimumWidth(200);

    //! assembly components
    splitter->addWidget(ui_sidebar);
    splitter->addWidget(ui_editor);

    layout->addWidget(ui_title_bar);
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

    //! config properties
    struct {
        const QColor Window          = QColor(60, 60, 60);
        const QColor WindowText      = QColor(160, 160, 160);
        const QColor Frame           = QColor(38, 38, 38);
        const QColor Base            = QColor(30, 30, 30);
        const QColor Text            = QColor(255, 255, 255);
        const QColor Highlight       = QColor(50, 100, 150, 150);
        const QColor Hover           = QColor(255, 255, 255, 30);
        const QColor HighlightedText = QColor(255, 255, 255, 10);
        const QColor SelectedText    = QColor(60, 60, 255, 80);
    } Palette;

    pal = ui_editor->palette();
    pal.setColor(QPalette::Window, Palette.Window);
    pal.setColor(QPalette::WindowText, Palette.WindowText);
    ui_title_bar->setPalette(pal);
    ui_status_bar->setPalette(pal);

    pal = ui_editor->palette();
    pal.setColor(QPalette::Window, Palette.Base);
    pal.setColor(QPalette::Text, Palette.Text);
    pal.setColor(QPalette::Highlight, Palette.HighlightedText);
    pal.setColor(QPalette::HighlightedText, Palette.SelectedText);
    ui_editor->setPalette(pal);

    pal = ui_sidebar->palette();
    pal.setColor(QPalette::Window, Palette.Frame);
    pal.setColor(QPalette::WindowText, Palette.WindowText);
    pal.setColor(QPalette::Base, Palette.Frame);
    pal.setColor(QPalette::Button, Palette.Window);
    pal.setColor(QPalette::Text, Palette.WindowText);
    pal.setColor(QPalette::ButtonText, Palette.WindowText);
    ui_sidebar->setPalette(pal);

    pal = ui_book_dir->palette();
    pal.setColor(QPalette::Highlight, Palette.Hover);
    ui_book_dir->setPalette(pal);

    pal = palette();
    pal.setColor(QPalette::Window, Palette.Base);
    pal.setColor(QPalette::WindowText, Palette.WindowText);
    setPalette(pal);

    font = ui_title_bar->font();
    font.setPointSize(10);
    ui_title_bar->setFont(font);
    ui_title_bar->setTitle("只写 丶 阐释你的梦");

    font = ui_status_bar->font();
    font.setPointSize(10);
    ui_status_bar->setFont(font);
    ui_status_bar->setSpacing(20);
    ui_status_bar->setContentsMargins(12, 4, 12, 4);
    ui_total_words = ui_status_bar->addItem("全本共 0 字", false);
    ui_datetime    = ui_status_bar->addItem("0000-00-00", true);

    content->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

    //! install frameless agent
    ui_agent = new QWK::WidgetWindowAgent(this);
    ui_agent->setup(this);
    ui_agent->setTitleBar(ui_title_bar);

    //! install book dir item render proxy
    ui_book_dir->setItemRenderProxy(new BookDirItemRenderProxy(this));

    setAutoFillBackground(true);
}

bool JustWrite::eventFilter(QObject *obj, QEvent *event) {
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
