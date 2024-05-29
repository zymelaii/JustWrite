#pragma once

#include <QObject>
#include <QAction>
#include <magic_enum.hpp>

namespace jwrite {

class AppAction : public QObject {
    Q_OBJECT

public:
    enum Action {
        OpenBookGallery,
        OpenEditor,
        OpenSettings,
        OpenHelp,
        OpenFavorites,
        OpenTrashBin,
        Share,
        Export,
        Fullscreen,
        ExitFullscreen,
    };

public:
    static AppAction& get_instance();

    QAction* get(Action action) const {
        return actions_[action];
    }

    template <typename Sender, typename Slot>
    void bind(Action action, Sender&& context, Slot slot) const {
        const auto sender = get(action);
        disconnect(sender, &QAction::triggered, nullptr, nullptr);
        connect(sender, &QAction::triggered, context, slot);
    }

    template <typename Sender, typename Signal>
    void attach(Action action, Sender&& sender, Signal signal) const {
        const auto receiver = get(action);
        connect(sender, signal, receiver, &QAction::trigger);
    }

    void detach(Action action, const QObject* sender) const {
        const auto receiver = get(action);
        disconnect(sender, nullptr, receiver, nullptr);
    }

public:
    AppAction();
    ~AppAction() override;

private:
    std::array<QAction*, magic_enum::enum_count<Action>()> actions_;
};

} // namespace jwrite
