#include <jwrite/TextEditHistory.h>
#include <magic_enum.hpp>

namespace jwrite {

TextEditAction TextEditAction::from_action(Type type, TextLoc loc, const QString& text) {
    TextEditAction action{};
    action.type = type == Type::Insert ? Type::Delete : Type::Insert;
    action.loc  = loc;
    action.text = text;
    return action;
}

TextEditHistory::TextEditHistory()
    : max_size_(256) {
    clear();
}

void TextEditHistory::set_max_size(size_t size) {
    Q_ASSERT(cursor_ >= -1 && cursor_ < static_cast<int>(rev_actions_.size()));
    if (const int len = static_cast<int>(rev_actions_.size()); size == 0) {
        clear();
    } else if (size < len) {
        const int total_redo   = len - (cursor_ + 1);
        const int total_undo   = cursor_ + 1;
        const int redo_removed = size - qMin<int>(total_redo, size);

        size -= redo_removed;
        if (const auto it = rev_actions_.end(); redo_removed) {
            rev_actions_.erase(it - redo_removed, it);
        }

        const int undo_removed = size;
        if (const auto it = rev_actions_.begin(); undo_removed > 0) {
            rev_actions_.erase(it, it + undo_removed);
        }

        if (cursor_ != -1) { cursor_ -= undo_removed; }
    }
    max_size_ = size;
}

void TextEditHistory::push(const TextEditAction& action) {
    Q_ASSERT(rev_actions_.size() <= max_size_);
    if (cursor_ + 1 < rev_actions_.size()) {
        rev_actions_.erase(rev_actions_.begin() + cursor_ + 1, rev_actions_.end());
    }
    Q_ASSERT(cursor_ + 1 == rev_actions_.size());
    rev_actions_.push_back(action);
    if (rev_actions_.size() == max_size_ + 1) { rev_actions_.pop_front(); }
    cursor_ = static_cast<int>(rev_actions_.size()) - 1;
}

std::optional<TextEditAction> TextEditHistory::get_undo_action() {
    Q_ASSERT(cursor_ >= -1 && cursor_ < static_cast<int>(rev_actions_.size()));
    if (cursor_ == -1) { return std::nullopt; }
    return rev_actions_[cursor_--];
}

std::optional<TextEditAction> TextEditHistory::get_redo_action() {
    using Type = TextEditAction::Type;
    Q_ASSERT(cursor_ >= -1 && cursor_ < static_cast<int>(rev_actions_.size()));
    if (cursor_ + 1 == rev_actions_.size()) { return std::nullopt; }
    auto action = rev_actions_[++cursor_];
    action.type = action.type == Type::Insert ? Type::Delete : Type::Insert;
    return action;
}

void TextEditHistory::clear() {
    rev_actions_.clear();
    cursor_ = -1;
}

QDebug operator<<(QDebug stream, const TextEditAction& action) {
    QDebugStateSaver saver(stream);
    stream.nospace() << "TextEditAction(" << magic_enum::enum_name(action.type) << ", "
                     << action.loc << ", " << action.text.length() << ")";
    return stream;
}

} // namespace jwrite
