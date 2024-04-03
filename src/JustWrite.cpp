#include "JustWrite.h"
#include "LimitedViewEditor.h"
#include "KeyShortcut.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QKeyEvent>

namespace Ui {
struct JustWrite {
    QHBoxLayout       *layout;
    LimitedViewEditor *editor;

    JustWrite(::JustWrite *parent) {
        layout = new QHBoxLayout(parent);
        editor = new LimitedViewEditor;

        layout->addWidget(editor);

        layout->setSpacing(0);
        layout->setContentsMargins({});

        auto pal = editor->palette();
        pal.setColor(QPalette::Base, Qt::transparent);
        pal.setColor(QPalette::Text, Qt::white);
        pal.setColor(QPalette::Highlight, QColor(50, 100, 150, 150));
        editor->setPalette(pal);
    }
};
} // namespace Ui

struct JustWritePrivate {
    KeyShortcut shortcut;

    JustWritePrivate() {
        shortcut.loadDefaultShortcuts();
    }
};

QKeySequence mkkeyseq(QKeyEvent *e) {
    return QKeySequence(e->key() | e->modifiers());
}

JustWrite::JustWrite(QWidget *parent)
    : QWidget(parent)
    , d{new JustWritePrivate}
    , ui{new Ui::JustWrite(this)} {
    ui->editor->installEventFilter(this);
    ui->editor->setFocus();
}

JustWrite::~JustWrite() {
    delete d;
    delete ui;
}

bool JustWrite::eventFilter(QObject *obj, QEvent *event) {
    if (event->type() == QEvent::KeyPress) {
        auto e = static_cast<QKeyEvent *>(event);
        if (mkkeyseq(e) == d->shortcut.toggle_align_center) {
            ui->editor->setAlignCenter(!ui->editor->alignCenter());
        }
    }
    return false;
}
