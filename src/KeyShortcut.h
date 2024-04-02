#pragma once

#include <QKeySequence>

struct KeyShortcut {
    QKeySequence toggle_align_center;

    void loadDefaultShortcuts();
};
