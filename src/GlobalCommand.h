#pragma once

#include <QKeyEvent>
#include <QMap>
#include <optional>

enum class GlobalCommand {
    ToggleSidebar,
    ToggleSoftCenterMode,
    DEV_EnableMessyInput,
};

class GlobalCommandManager {
public:
    void load_default();
    void clear();
    void insert_or_update(QKeySequence key, GlobalCommand cmd);

    std::optional<GlobalCommand> match(QKeyEvent* e) const;

private:
    QMap<QKeySequence, GlobalCommand> shortcuts_;
};
