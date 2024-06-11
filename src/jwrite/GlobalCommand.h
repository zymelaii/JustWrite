#pragma once

#include <QKeyEvent>
#include <QMap>
#include <optional>

namespace jwrite {

enum class GlobalCommand {
    ToggleFullscreen,
    ToggleSidebar,
    ToggleSoftCenterMode,
    CreateNewChapter,
    Rename,
};

class GlobalCommandManager {
public:
    void         load_default();
    void         clear();
    void         insert_or_update(QKeySequence key, GlobalCommand cmd);
    void         update_command_shortcut(GlobalCommand cmd, QKeySequence new_shortcut);
    QKeySequence get(GlobalCommand command) const;

    std::optional<GlobalCommand> match(QKeyEvent* e) const;

    static GlobalCommandManager& get_instance();

private:
    QMap<QKeySequence, GlobalCommand> shortcuts_;
};

} // namespace jwrite
