#pragma once

#include <QAbstractNativeEventFilter>

namespace jwrite {

class InputMethodHook : public QAbstractNativeEventFilter {
public:
    bool nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result) override;
};

} // namespace jwrite
