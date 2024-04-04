#pragma once

#include <QKeySequence>
#include <QKeyEvent>

struct KeyShortcut {
    QKeySequence toggle_align_center;

    void loadDefaultShortcuts();

    bool matches(QKeyEvent *e, QKeySequence::StandardKey key);
    bool matches(QKeyEvent *e, QKeySequence key);
    bool matches(QKeyEvent *e, const QString& key);
};
