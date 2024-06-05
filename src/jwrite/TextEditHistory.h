#pragma once

#include <jwrite/VisualTextEditContext.h>
#include <QVariant>
#include <deque>
#include <stddef.h>
#include <optional>

namespace jwrite {

struct TextEditAction {
    using TextLoc = VisualTextEditContext::TextLoc;

    enum Type {
        Insert,
        Delete,
    };

    static TextEditAction from_action(Type type, TextLoc loc, const QString& text);

    Type    type;
    TextLoc loc;
    QString text;
};

class TextEditHistory {
public:
    TextEditHistory();

    int cursor() const {
        return cursor_;
    }

    size_t total_records() const {
        return rev_actions_.size();
    }

    size_t max_size() const {
        return max_size_;
    }

    const TextEditAction& current_record() const {
        return rev_actions_.at(cursor_);
    }

    void set_max_size(size_t size);

    void push(const TextEditAction& action);

    [[nodiscard]] std::optional<TextEditAction> get_undo_action();
    [[nodiscard]] std::optional<TextEditAction> get_redo_action();

    void clear();

private:
    size_t                     max_size_;
    std::deque<TextEditAction> rev_actions_;
    int                        cursor_;
};

QDebug operator<<(QDebug stream, const TextEditAction& action);

} // namespace jwrite
