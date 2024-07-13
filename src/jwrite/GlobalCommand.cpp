#include <jwrite/GlobalCommand.h>

namespace jwrite {

void GlobalCommandManager::load_default() {
    clear();

#define KEYBINDING(cmd, key_name) \
    insert_or_update(QKeySequence::fromString(key_name), GlobalCommand::cmd)

    KEYBINDING(ToggleFullscreen, "F11");
    KEYBINDING(ToggleSidebar, "Ctrl+Alt+B");
    KEYBINDING(ToggleSoftCenterMode, "Ctrl+E");
    KEYBINDING(CreateNewChapter, "Ctrl+N");
    KEYBINDING(Rename, "F2");
    KEYBINDING(PopContextMenu, "Menu");

#undef KEYBINDING
}

void GlobalCommandManager::clear() {
    shortcuts_.clear();
}

void GlobalCommandManager::insert_or_update(QKeySequence key, GlobalCommand cmd) {
    shortcuts_[key] = cmd;
}

void GlobalCommandManager::update_command_shortcut(GlobalCommand cmd, QKeySequence new_shortcut) {
    QKeySequence old_shortcut{};
    for (const auto &[shortcut, cmd_ref] : shortcuts_.asKeyValueRange()) {
        if (cmd_ref == cmd) {
            old_shortcut = shortcut;
            break;
        }
    }
    if (!old_shortcut.isEmpty()) { shortcuts_.remove(old_shortcut); }
    insert_or_update(new_shortcut, cmd);
}

QKeySequence GlobalCommandManager::get(GlobalCommand command) const {
    for (const auto &[shortcut, cmd] : shortcuts_.asKeyValueRange()) {
        if (cmd == command) { return shortcut; }
    }
    return QKeySequence();
}

std::optional<GlobalCommand> GlobalCommandManager::match(QKeyEvent *e) const {
    QKeySequence key(e->modifiers() | e->key());
    if (shortcuts_.contains(key)) { return {shortcuts_[key]}; }
    return std::nullopt;
}

} // namespace jwrite
