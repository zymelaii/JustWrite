#include <jwrite/GlobalCommand.h>
#include <memory>

namespace jwrite {

GlobalCommandManager &GlobalCommandManager::get_instance() {
    static std::unique_ptr<GlobalCommandManager> instance{};
    if (!instance) {
        instance.reset(new GlobalCommandManager);
        instance->load_default();
    }
    return *instance;
}

void GlobalCommandManager::load_default() {
    clear();

#define KEYBINDING(cmd, key_name) \
    insert_or_update(QKeySequence::fromString(key_name), GlobalCommand::cmd)

    KEYBINDING(ToggleFullscreen, "F11");
    KEYBINDING(ToggleSidebar, "Ctrl+Alt+B");
    KEYBINDING(ToggleSoftCenterMode, "Ctrl+E");
    KEYBINDING(CreateNewChapter, "Ctrl+N");
    KEYBINDING(Rename, "F2");

#undef KEYBINDING
}

void GlobalCommandManager::clear() {
    shortcuts_.clear();
}

void GlobalCommandManager::insert_or_update(QKeySequence key, GlobalCommand cmd) {
    shortcuts_[key] = cmd;
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
