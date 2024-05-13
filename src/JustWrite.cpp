#include "JustWrite.h"
#include "LimitedViewEditor.h"
#include "JustWriteSidebar.h"
#include "StatusBar.h"
#include "GlobalCommand.h"
#include "TextInputCommand.h"
#include "ProfileUtils.h"
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

#ifdef WIN32

#include <windows.h>
#include <QThread>

std::atomic_bool develop_messy_mode{false};

class DevelopModeMessyTest : public QThread {
public:
    explicit DevelopModeMessyTest(QObject *parent = nullptr)
        : QThread(parent) {}

protected:
    void sendKey(int key) {
        keybd_event(key, 0, 0, 0);
        keybd_event(key, 0, KEYEVENTF_KEYUP, 0);
    }

    void run() override {
        std::random_device{};
        std::mt19937                       rng{std::random_device{}()};
        std::uniform_int_distribution<int> type_dist{1, 12};
        std::uniform_int_distribution<int> char_dist{'A', 'Z'};
        std::uniform_int_distribution<int> nl_dist{0, 4};

        while (true) {
            if (!develop_messy_mode) { continue; }

            const int type_times = type_dist(rng);
            for (int j = 0; j < type_times; ++j) {
                if (!develop_messy_mode) { break; }
                const auto ch = char_dist(rng);
                sendKey(ch);
                Sleep(1);
            }

            if (!develop_messy_mode) { continue; }
            sendKey(VK_SPACE);
            Sleep(10);

            if (nl_dist(rng) == 0) {
                if (!develop_messy_mode) { continue; }
                sendKey(VK_RETURN);
                Sleep(10);
            }
        }
    }
};

DevelopModeMessyTest *messy_input_thread = nullptr;

#endif

namespace Ui {
struct JustWrite {
    jwrite::StatusBar  *status_bar;
    ::JustWriteSidebar *sidebar;
    LimitedViewEditor  *editor;
    QLabel             *total_words;
    QLabel             *datetime;

    JustWrite(::JustWrite *parent) {
        auto layout         = new QVBoxLayout(parent);
        auto content        = new QWidget;
        auto layout_content = new QHBoxLayout(content);
        auto splitter       = new QSplitter;
        status_bar          = new jwrite::StatusBar;
        sidebar             = new ::JustWriteSidebar(splitter);
        editor              = new LimitedViewEditor(splitter);

        layout->addWidget(content);
        layout->addWidget(status_bar);
        layout->setContentsMargins({});
        layout->setSpacing(0);

        layout_content->addWidget(splitter);
        layout_content->setContentsMargins({});

        auto pal = editor->palette();
        pal.setColor(QPalette::Window, QColor(30, 30, 30));
        pal.setColor(QPalette::Text, Qt::white);
        pal.setColor(QPalette::Highlight, QColor(50, 100, 150, 150));
        editor->setPalette(pal);

        pal = sidebar->palette();
        pal.setColor(QPalette::Window, QColor(38, 38, 38));
        pal.setColor(QPalette::WindowText, Qt::white);
        pal.setColor(QPalette::Base, QColor(38, 38, 38));
        pal.setColor(QPalette::Button, QColor(55, 55, 55));
        pal.setColor(QPalette::Text, Qt::white);
        pal.setColor(QPalette::ButtonText, Qt::white);
        sidebar->setPalette(pal);

        pal = parent->palette();
        pal.setColor(QPalette::Window, QColor(30, 30, 30));
        pal.setColor(QPalette::WindowText, Qt::white);
        parent->setPalette(pal);

        splitter->setOrientation(Qt::Horizontal);
        splitter->setStretchFactor(0, 0);
        splitter->setStretchFactor(1, 1);
        splitter->setCollapsible(0, false);
        splitter->setCollapsible(1, false);

        content->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

        auto font = status_bar->font();
        font.setPointSize(10);
        pal.setColor(QPalette::Window, QColor(0, 120, 200));
        status_bar->setPalette(pal);
        status_bar->setFont(font);
        status_bar->setSpacing(20);
        status_bar->setContentsMargins(12, 4, 12, 4);
        total_words = status_bar->addItem("全本共 0 字");
        datetime    = status_bar->addItemAtRightSide("0000-00-00");

        parent->setAutoFillBackground(true);
    }
};
} // namespace Ui

struct JustWritePrivate {
    GlobalCommandManager     command_manager;
    QTimer                   sec_timer;
    int                      current_cid;
    QMap<int, QString>       chapters;
    QMap<int, EditorTextLoc> chapter_locs;

    JustWritePrivate() {
        command_manager.loadDefaultMappings();
        sec_timer.setInterval(1000);
        sec_timer.setSingleShot(false);

        current_cid = -1;
    }
};

JustWrite::JustWrite(QWidget *parent)
    : QWidget(parent)
    , d{new JustWritePrivate}
    , ui{new Ui::JustWrite(this)} {
    ui->editor->installEventFilter(this);
    ui->editor->setFocus();

    connect(
        ui->editor,
        &LimitedViewEditor::requireEmptyChapter,
        ui->sidebar,
        &JustWriteSidebar::openEmptyChapter);
    connect(ui->editor, &LimitedViewEditor::textChanged, this, [this](const QString &text) {
        ui->total_words->setText(QString("全本共 %1 字").arg(text.length()));
    });
    connect(ui->sidebar, &JustWriteSidebar::chapterOpened, this, &JustWrite::openChapter);
    connect(&d->sec_timer, &QTimer::timeout, this, [this] {
        ui->datetime->setText(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"));
    });

    d->sec_timer.start();

#ifdef WIN32
    connect(ui->editor, &LimitedViewEditor::focusLost, this, [] {
        develop_messy_mode = false;
    });
    messy_input_thread = new DevelopModeMessyTest(this);
    messy_input_thread->start();
#endif
}

JustWrite::~JustWrite() {
#ifdef WIN32
    messy_input_thread->terminate();
    messy_input_thread->wait();
#endif

    delete d;
    delete ui;
}

void JustWrite::openChapter(int cid) {
#ifdef WIN32
    if (develop_messy_mode) { return; }
#endif
    if (cid == d->current_cid) { return; }

    JwriteProfilerStart(SwitchChapter);

    QString  tmp{};
    QString &text = d->chapters.contains(cid) ? d->chapters[cid] : tmp;

    if (d->current_cid != -1) {
        const auto loc = ui->editor->textLoc();
        if (loc.block_index != -1) { d->chapter_locs[d->current_cid] = loc; }
    }

    ui->editor->reset(text, true);
    d->chapters[d->current_cid] = text;
    d->current_cid              = cid;

    if (d->chapter_locs.contains(cid)) {
        const auto loc = d->chapter_locs[cid];
        ui->editor->setTextLoc(loc);
    }

    JwriteProfilerRecord(SwitchChapter);
}

bool JustWrite::eventFilter(QObject *obj, QEvent *event) {
    if (event->type() == QEvent::KeyPress) {
        auto e = static_cast<QKeyEvent *>(event);

#ifdef WIN32
        //! NOTE: block special keys in messy input mode to avoid unexpceted behavior
        //! ATTENTION: this can not block global shortcut keys
        if (const auto key = QKeyCombination::fromCombined(e->key() | e->modifiers());
            !TextInputCommandManager::isPrintableChar(key) && develop_messy_mode) {
            return true;
        }
#endif

        if (auto opt = d->command_manager.match(e)) {
            const auto action = *opt;
            qDebug() << "COMMAND" << magic_enum::enum_name(action).data();

            switch (action) {
                case GlobalCommand::ToggleSidebar: {
                    ui->sidebar->setVisible(!ui->sidebar->isVisible());
                } break;
                case GlobalCommand::ToggleSoftCenterMode: {
                    ui->editor->setAlignCenter(!ui->editor->alignCenter());
                } break;
                case GlobalCommand::DEV_EnableMessyInput: {
#ifdef WIN32
                    ui->editor->setFocus();
                    develop_messy_mode = true;
#endif
                } break;
            }
            return true;
        }
    }

    return false;
}
