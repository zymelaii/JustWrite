#include "KeyShortcut.h"

void KeyShortcut::loadDefaultShortcuts() {
    toggle_align_center = QKeySequence::fromString("Ctrl+E");
    toggle_sidebar      = QKeySequence::fromString("Ctrl+Alt+B");
}

bool KeyShortcut::matches(QKeyEvent *e, QKeySequence::StandardKey key) {
    const auto key_input =
        (e->modifiers() | e->key()) & ~(Qt::KeypadModifier | Qt::GroupSwitchModifier);
    const auto bindings = QKeySequence::keyBindings(key);
    return bindings.contains(QKeySequence(key_input));
}

bool KeyShortcut::matches(QKeyEvent *e, QKeySequence key) {
    const auto key_input =
        (e->modifiers() | e->key()) & ~(Qt::KeypadModifier | Qt::GroupSwitchModifier);
    return key_input == key;
}

bool KeyShortcut::matches(QKeyEvent *e, const QString &key) {
    return matches(e, QKeySequence::fromString(key));
}
