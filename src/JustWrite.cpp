#include "JustWrite.h"
#include "LimitedViewEditor.h"
#include "StatusBar.h"
#include "GlobalCommand.h"
#include "TextInputCommand.h"
#include "FlatButton.h"
#include "CatalogTree.h"
#include "TitleBar.h"
#include "ProfileUtils.h"
#include "MessyInput.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QSplitter>
#include <QStatusBar>
#include <QLabel>
#include <QDateTime>
#include <QTimer>
#include <atomic>
#include <random>
#include <QMap>
#include <QFrame>
#include <QWKWidgets/widgetwindowagent.h>

namespace Ui {
struct JustWrite {
    jwrite::TitleBar  *title_bar;
    LimitedViewEditor *editor;
    jwrite::StatusBar *status_bar;
    QLabel            *total_words;
    QLabel            *datetime;

    //! left sidebar
    QWidget       *sidebar;
    FlatButton    *new_volume;
    FlatButton    *new_chapter;
    ::CatalogTree *book_dir;

    QWK::WidgetWindowAgent *agent;

    void setup(::JustWrite *parent) {
        auto layout         = new QVBoxLayout(parent);
        auto content        = new QWidget;
        auto layout_content = new QHBoxLayout(content);
        auto splitter       = new QSplitter;
        title_bar           = new jwrite::TitleBar;
        editor              = new LimitedViewEditor;
        status_bar          = new jwrite::StatusBar;

        //! setup left sidebar
        {
            sidebar     = new QWidget;
            new_volume  = new FlatButton;
            new_chapter = new FlatButton;
            book_dir    = new ::CatalogTree;

            auto btn_line        = new QWidget;
            auto btn_line_layout = new QHBoxLayout(btn_line);
            btn_line_layout->setContentsMargins({});
            btn_line_layout->addStretch();
            btn_line_layout->addWidget(new_volume);
            btn_line_layout->addStretch();
            btn_line_layout->addWidget(new_chapter);
            btn_line_layout->addStretch();

            auto sep_line = new QFrame;
            sep_line->setFrameShape(QFrame::HLine);

            auto sidebar_layout = new QVBoxLayout(sidebar);
            sidebar_layout->setContentsMargins(0, 10, 0, 10);
            sidebar_layout->addWidget(btn_line);
            sidebar_layout->addWidget(sep_line);
            sidebar_layout->addWidget(book_dir);

            auto font = parent->font();
            font.setPointSize(10);
            sidebar->setFont(font);

            auto pal = parent->palette();
            pal.setColor(QPalette::WindowText, QColor(70, 70, 70));
            sep_line->setPalette(pal);

            new_volume->setContentsMargins(10, 0, 10, 0);
            new_volume->set_text(QObject::tr("新建卷"));
            new_volume->set_text_alignment(Qt::AlignCenter);
            new_volume->set_radius(6);

            new_chapter->setContentsMargins(10, 0, 10, 0);
            new_chapter->set_text(QObject::tr("新建章"));
            new_chapter->set_text_alignment(Qt::AlignCenter);
            new_chapter->set_radius(6);

            sidebar->setAutoFillBackground(true);
            sidebar->setMinimumWidth(200);
        }

        splitter->addWidget(sidebar);
        splitter->addWidget(editor);

        layout->addWidget(title_bar);
        layout->addWidget(content);
        layout->addWidget(status_bar);
        layout->setContentsMargins({});
        layout->setSpacing(0);

        layout_content->addWidget(splitter);
        layout_content->setContentsMargins({});

        struct {
            const QColor Window     = QColor(60, 60, 60);
            const QColor WindowText = QColor(160, 160, 160);
            const QColor Frame      = QColor(38, 38, 38);
            const QColor Base       = QColor(30, 30, 30);
            const QColor Text       = QColor(255, 255, 255);
            const QColor Highlight  = QColor(50, 100, 150, 150);
        } Palette;

        auto pal = editor->palette();
        pal.setColor(QPalette::Window, Palette.Window);
        pal.setColor(QPalette::WindowText, Palette.WindowText);
        title_bar->setPalette(pal);
        status_bar->setPalette(pal);

        pal = editor->palette();
        pal.setColor(QPalette::Window, Palette.Base);
        pal.setColor(QPalette::Text, Palette.Text);
        pal.setColor(QPalette::Highlight, Palette.Highlight);
        editor->setPalette(pal);

        pal = sidebar->palette();
        pal.setColor(QPalette::Window, Palette.Frame);
        pal.setColor(QPalette::WindowText, Palette.WindowText);
        pal.setColor(QPalette::Base, Palette.Frame);
        pal.setColor(QPalette::Button, Palette.Window);
        pal.setColor(QPalette::Text, Palette.WindowText);
        pal.setColor(QPalette::ButtonText, Palette.WindowText);
        sidebar->setPalette(pal);

        pal = parent->palette();
        pal.setColor(QPalette::Window, Palette.Base);
        pal.setColor(QPalette::WindowText, Palette.WindowText);
        parent->setPalette(pal);

        splitter->setOrientation(Qt::Horizontal);
        splitter->setStretchFactor(0, 0);
        splitter->setStretchFactor(1, 1);
        splitter->setCollapsible(0, false);
        splitter->setCollapsible(1, false);

        content->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

        auto font = title_bar->font();
        font.setPointSize(10);
        title_bar->setFont(font);
        title_bar->set_title("只写 丶 阐释你的梦");

        font = status_bar->font();
        font.setPointSize(10);
        status_bar->setFont(font);
        status_bar->set_spacing(20);
        status_bar->setContentsMargins(12, 4, 12, 4);
        total_words = status_bar->add_item("全本共 0 字", false);
        datetime    = status_bar->add_item("0000-00-00", true);

        parent->setAutoFillBackground(true);

        agent = new QWK::WidgetWindowAgent(parent);
        agent->setup(parent);
        agent->setTitleBar(title_bar);
    }
};
} // namespace Ui

struct JustWritePrivate {
    GlobalCommandManager     command_manager;
    QTimer                   sec_timer;
    int                      current_cid;
    QMap<int, QString>       chapters;
    QMap<int, EditorTextLoc> chapter_locs;

    MessyInputWorker *messy_input;

    JustWritePrivate() {
        command_manager.load_default();
        sec_timer.setInterval(1000);
        sec_timer.setSingleShot(false);

        current_cid = -1;
    }

    ~JustWritePrivate() {
        if (messy_input) {
            messy_input->kill();
            delete messy_input;
        }
    }
};

JustWrite::JustWrite(QWidget *parent)
    : QWidget(parent)
    , d{new JustWritePrivate}
    , ui{new Ui::JustWrite} {
    ui->setup(this);
    ui->editor->installEventFilter(this);
    ui->editor->setFocus();

    d->messy_input = new MessyInputWorker(this);

    connect(
        ui->editor, &LimitedViewEditor::requireEmptyChapter, this, &JustWrite::open_empty_chapter);
    connect(ui->editor, &LimitedViewEditor::textChanged, this, [this](const QString &text) {
        ui->total_words->setText(QString("全本共 %1 字").arg(text.length()));
    });
    connect(ui->book_dir, &CatalogTree::itemClicked, this, [this](int vid, int cid) {
        open_chapter(cid);
    });
    connect(ui->new_volume, &FlatButton::pressed, this, [this] {
        create_new_volume(-1, "");
    });
    connect(ui->new_chapter, &FlatButton::pressed, this, [this] {
        create_new_chapter(-1, "");
    });
    connect(&d->sec_timer, &QTimer::timeout, this, [this] {
        ui->datetime->setText(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"));
    });
    connect(ui->editor, &LimitedViewEditor::focusLost, this, [this] {
        d->messy_input->kill();
    });

    d->sec_timer.start();
}

JustWrite::~JustWrite() {
    delete d;
    delete ui;
}

void JustWrite::create_new_volume(int index, const QString &title) {
    if (index == -1) { index = ui->book_dir->get_total_volumes(); }
    ui->book_dir->add_volume(index, title);
}

void JustWrite::create_new_chapter(int volume_index, const QString &title) {
    if (volume_index == -1) { volume_index = ui->book_dir->get_total_volumes() - 1; }
    const auto vid = ui->book_dir->get_volume(volume_index);
    Q_ASSERT(vid != -1);
    int cid = ui->book_dir->add_chapter(vid, title);
    open_chapter(cid);
}

void JustWrite::open_empty_chapter() {
    const auto vid = ui->book_dir->get_volume(0);
    if (vid == -1) { create_new_volume(0, ""); }
    create_new_chapter(-1, "");
}

void JustWrite::open_chapter(int cid) {
    if (d->messy_input->is_running()) { return; }

    if (cid == d->current_cid) { return; }

    jwrite_profiler_start(SwitchChapter);

    QString  tmp{};
    QString &text = d->chapters.contains(cid) ? d->chapters[cid] : tmp;

    if (d->current_cid != -1) {
        const auto loc = ui->editor->get_current_text_loc();
        if (loc.block_index != -1) { d->chapter_locs[d->current_cid] = loc; }
    }

    ui->editor->reset(text, true);
    d->chapters[d->current_cid] = text;
    d->current_cid              = cid;

    if (d->chapter_locs.contains(cid)) {
        const auto loc = d->chapter_locs[cid];
        ui->editor->update_text_loc(loc);
    }

    jwrite_profiler_record(SwitchChapter);
}

bool JustWrite::eventFilter(QObject *obj, QEvent *event) {
    if (event->type() == QEvent::KeyPress) {
        auto e = static_cast<QKeyEvent *>(event);

        //! NOTE: block special keys in messy input mode to avoid unexpceted behavior
        //! ATTENTION: this can not block global shortcut keys
        if (const auto key = QKeyCombination::fromCombined(e->key() | e->modifiers());
            !TextInputCommandManager::is_printable_char(key) && d->messy_input->is_running()) {
            return true;
        }

        if (auto opt = d->command_manager.match(e)) {
            const auto action = *opt;
            qDebug() << "COMMAND" << magic_enum::enum_name(action).data();

            switch (action) {
                case GlobalCommand::ToggleSidebar: {
                    ui->sidebar->setVisible(!ui->sidebar->isVisible());
                } break;
                case GlobalCommand::ToggleSoftCenterMode: {
                    ui->editor->set_soft_center_mode(!ui->editor->is_soft_center_mode());
                } break;
                case GlobalCommand::DEV_EnableMessyInput: {
                    ui->editor->setFocus();
                    d->messy_input->start();
                } break;
            }
            return true;
        }
    }

    if (event->type() == QEvent::MouseButtonDblClick) {
        auto e = static_cast<QMouseEvent *>(event);
        if (e->buttons() == Qt::RightButton) {
            d->messy_input->kill();
            return false;
        }
    }

    return false;
}
