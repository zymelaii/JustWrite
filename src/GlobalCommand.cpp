#include "GlobalCommand.h"

void GlobalCommandManager::loadDefaultMappings() {
    clear();

#define KEYBINDING(cmd, key_name) \
    insertOrUpdateKeyBinding(QKeySequence::fromString(key_name), GlobalCommand::cmd)

    KEYBINDING(ToggleSidebar, "Ctrl+Alt+B");
    KEYBINDING(ToggleSoftCenterMode, "Ctrl+E");
    KEYBINDING(DEV_EnableMessyInput, "Ctrl+F12");

#undef KEYBINDING
}

void GlobalCommandManager::clear() {
    shortcuts_.clear();
}

void GlobalCommandManager::insertOrUpdateKeyBinding(QKeySequence key, GlobalCommand cmd) {
    shortcuts_[key] = cmd;
}

std::optional<GlobalCommand> GlobalCommandManager::match(QKeyEvent* e) const {
    QKeySequence key(e->modifiers() | e->key());
    if (shortcuts_.contains(key)) { return {shortcuts_[key]}; }
    return std::nullopt;
}
