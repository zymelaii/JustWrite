#include <jwrite/GlobalCommand.h>

namespace jwrite {

void GlobalCommandManager::load_default() {
    clear();

#define KEYBINDING(cmd, key_name) \
    insert_or_update(QKeySequence::fromString(key_name), GlobalCommand::cmd)

    KEYBINDING(ToggleSidebar, "Ctrl+Alt+B");
    KEYBINDING(ToggleSoftCenterMode, "Ctrl+E");
    KEYBINDING(CreateNewChapter, "Ctrl+N");
    KEYBINDING(Rename, "F2");
    KEYBINDING(ShowColorSchemeDialog, "Ctrl+`");
    KEYBINDING(DEV_EnableMessyInput, "F12");

#undef KEYBINDING
}

void GlobalCommandManager::clear() {
    shortcuts_.clear();
}

void GlobalCommandManager::insert_or_update(QKeySequence key, GlobalCommand cmd) {
    shortcuts_[key] = cmd;
}

std::optional<GlobalCommand> GlobalCommandManager::match(QKeyEvent* e) const {
    QKeySequence key(e->modifiers() | e->key());
    if (shortcuts_.contains(key)) { return {shortcuts_[key]}; }
    return std::nullopt;
}

} // namespace jwrite
