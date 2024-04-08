#pragma once

#include <QKeySequence>
#include <QKeyEvent>

struct KeyShortcut {
    QKeySequence toggle_align_center;
    QKeySequence toggle_sidebar;

    void loadDefaultShortcuts();

    bool matches(QKeyEvent *e, QKeySequence::StandardKey key);
    bool matches(QKeyEvent *e, QKeySequence key);
    bool matches(QKeyEvent *e, const QString &key);
};
