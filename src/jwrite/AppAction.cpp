#include <jwrite/AppAction.h>
#include <memory>

namespace jwrite {

AppAction &AppAction::get_instance() {
    static std::unique_ptr<AppAction> instance{};
    if (!instance) { instance = std::make_unique<AppAction>(); }
    return *instance;
}

AppAction::AppAction()
    : QObject() {
    for (auto &action : actions_) { action = new QAction(this); }
}

AppAction::~AppAction() {}

} // namespace jwrite
