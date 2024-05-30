#include <jwrite/TextInputCommand.h>

namespace jwrite {

static QKeySequence
    get_default_keybinding(QKeySequence::StandardKey cmd, QKeySequence default_key) {
    const auto bindings = QKeySequence::keyBindings(cmd);
    if (bindings.isEmpty()) { return default_key; }
    //! FIXME: QKeySequence may have extended key combination
    return bindings.first()[0];
}

void TextInputCommandManager::load_default() {
    const auto InsertBeforeBlock       = QKeySequence::fromString("Ctrl+Shift+Return");
    const auto InsertAfterBlock        = QKeySequence::fromString("Ctrl+Return");
    const auto Cancel                  = QKeySequence::fromString("Escape");
    const auto Undo                    = QKeySequence::fromString("Ctrl+Z");
    const auto Redo                    = QKeySequence::fromString("Ctrl+Shift+Z");
    const auto Copy                    = QKeySequence::fromString("Ctrl+C");
    const auto Cut                     = QKeySequence::fromString("Ctrl+X");
    const auto Paste                   = QKeySequence::fromString("Ctrl+V");
    const auto ScrollUp                = QKeySequence::fromString("Ctrl+Up");
    const auto ScrollDown              = QKeySequence::fromString("Ctrl+Down");
    const auto MoveToPrevChar          = QKeySequence::fromString("Left");
    const auto MoveToNextChar          = QKeySequence::fromString("Right");
    const auto MoveToPrevWord          = QKeySequence::fromString("Ctrl+Left");
    const auto MoveToNextWord          = QKeySequence::fromString("Ctrl+Right");
    const auto MoveToPrevLine          = QKeySequence::fromString("Up");
    const auto MoveToNextLine          = QKeySequence::fromString("Down");
    const auto MoveToStartOfLine       = QKeySequence::fromString("Home");
    const auto MoveToEndOfLine         = QKeySequence::fromString("End");
    const auto MoveToStartOfBlock      = QKeySequence{};
    const auto MoveToEndOfBlock        = QKeySequence{};
    const auto MoveToStartOfDocument   = QKeySequence::fromString("Ctrl+Home");
    const auto MoveToEndOfDocument     = QKeySequence::fromString("Ctrl+End");
    const auto MoveToPrevPage          = QKeySequence::fromString("PageUp");
    const auto MoveToNextPage          = QKeySequence::fromString("PageDown");
    const auto MoveToPrevBlock         = QKeySequence::fromString("Shift+Alt+Return");
    const auto MoveToNextBlock         = QKeySequence::fromString("Alt+Return");
    const auto DeletePrevChar          = QKeySequence::fromString("Backspace");
    const auto DeleteNextChar          = QKeySequence::fromString("Delete");
    const auto DeletePrevWord          = QKeySequence::fromString("Ctrl+Backspace");
    const auto DeleteNextWord          = QKeySequence::fromString("Ctrl+Delete");
    const auto DeleteToStartOfLine     = QKeySequence::fromString("Ctrl+U");
    const auto DeleteToEndOfLine       = QKeySequence::fromString("Ctrl+K");
    const auto DeleteToStartOfBlock    = QKeySequence{};
    const auto DeleteToEndOfBlock      = QKeySequence{};
    const auto DeleteToStartOfDocument = QKeySequence{};
    const auto DeleteToEndOfDocument   = QKeySequence{};
    const auto SelectPrevChar          = QKeySequence::fromString("Shift+Left");
    const auto SelectNextChar          = QKeySequence::fromString("Shift+Right");
    const auto SelectPrevWord          = QKeySequence::fromString("Ctrl+Shift+Left");
    const auto SelectNextWord          = QKeySequence::fromString("Ctrl+Shift+Right");
    const auto SelectToPrevLine        = QKeySequence::fromString("Shift+Up");
    const auto SelectToNextLine        = QKeySequence::fromString("Shift+Down");
    const auto SelectToStartOfLine     = QKeySequence::fromString("Shift+Home");
    const auto SelectToEndOfLine       = QKeySequence::fromString("Shift+End");
    const auto SelectToStartOfBlock    = QKeySequence{};
    const auto SelectToEndOfBlock      = QKeySequence{};
    const auto SelectBlock             = QKeySequence::fromString("Ctrl+D");
    const auto SelectPrevPage          = QKeySequence::fromString("Shift+PageUp");
    const auto SelectNextPage          = QKeySequence::fromString("Shift+PageDown");
    const auto SelectToStartOfDoc      = QKeySequence::fromString("Ctrl+Shift+Home");
    const auto SelectToEndOfDoc        = QKeySequence::fromString("Ctrl+Shift+End");
    const auto SelectAll               = QKeySequence::fromString("Ctrl+A");

#define NARG(...)                        NARG_SELECT(__VA_ARGS__, 2, 1)
#define NARG_SELECT(...)                 NARG_ENUM(__VA_ARGS__)
#define NARG_ENUM(_1, _2, N, ...)        N
#define KEYBINDING(...)                  KEYBINDING_IMPL(NARG(__VA_ARGS__), __VA_ARGS__)
#define KEYBINDING_DISPATCH(N)           KEYBINDING_##N
#define KEYBINDING_IMPL(N, ...)          insert_or_update(KEYBINDING_DISPATCH(N)(__VA_ARGS__))
#define KEYBINDING_1(cmd)                cmd, TextInputCommand::cmd
#define KEYBINDING_2(key, cmd)           DEFAULT_KEYBINDING(key, cmd), TextInputCommand::cmd
#define DEFAULT_KEYBINDING(key, default) get_default_keybinding(QKeySequence::key, default)

    KEYBINDING(InsertBeforeBlock);
    KEYBINDING(InsertAfterBlock);
    KEYBINDING(Cancel, Cancel);
    KEYBINDING(Undo, Undo);
    KEYBINDING(Redo, Redo);
    KEYBINDING(Copy, Copy);
    KEYBINDING(Cut, Cut);
    KEYBINDING(Paste, Paste);
    KEYBINDING(ScrollUp);
    KEYBINDING(ScrollDown);
    KEYBINDING(MoveToPreviousChar, MoveToPrevChar);
    KEYBINDING(MoveToNextChar, MoveToNextChar);
    KEYBINDING(MoveToPreviousWord, MoveToPrevWord);
    KEYBINDING(MoveToNextWord, MoveToNextWord);
    KEYBINDING(MoveToPreviousLine, MoveToPrevLine);
    KEYBINDING(MoveToNextLine, MoveToNextLine);
    KEYBINDING(MoveToStartOfLine, MoveToStartOfLine);
    KEYBINDING(MoveToEndOfLine, MoveToEndOfLine);
    // KEYBINDING(MoveToStartOfBlock);
    // KEYBINDING(MoveToEndOfBlock);
    KEYBINDING(MoveToStartOfDocument, MoveToStartOfDocument);
    KEYBINDING(MoveToEndOfDocument, MoveToEndOfDocument);
    KEYBINDING(MoveToPreviousPage, MoveToPrevPage);
    KEYBINDING(MoveToNextPage, MoveToNextPage);
    KEYBINDING(MoveToPrevBlock);
    KEYBINDING(MoveToNextBlock);
    KEYBINDING(Backspace, DeletePrevChar);
    KEYBINDING(Delete, DeleteNextChar);
    KEYBINDING(DeleteStartOfWord, DeletePrevWord);
    KEYBINDING(DeleteEndOfWord, DeleteNextWord);
    KEYBINDING(DeleteToStartOfLine);
    KEYBINDING(DeleteToEndOfLine);
    // KEYBINDING(DeleteToStartOfBlock);
    // KEYBINDING(DeleteToEndOfBlock);
    // KEYBINDING(DeleteToStartOfDocument);
    // KEYBINDING(DeleteToEndOfDocument);
    KEYBINDING(SelectPreviousChar, SelectPrevChar);
    KEYBINDING(SelectNextChar, SelectNextChar);
    KEYBINDING(SelectPreviousWord, SelectPrevWord);
    KEYBINDING(SelectNextWord, SelectNextWord);
    KEYBINDING(SelectPreviousLine, SelectToPrevLine);
    KEYBINDING(SelectNextLine, SelectToNextLine);
    KEYBINDING(SelectStartOfLine, SelectToStartOfLine);
    KEYBINDING(SelectEndOfLine, SelectToEndOfLine);
    // KEYBINDING(SelectToStartOfBlock);
    // KEYBINDING(SelectToEndOfBlock);
    KEYBINDING(SelectBlock);
    KEYBINDING(SelectPreviousPage, SelectPrevPage);
    KEYBINDING(SelectNextPage, SelectNextPage);
    KEYBINDING(SelectStartOfDocument, SelectToStartOfDoc);
    KEYBINDING(SelectEndOfDocument, SelectToEndOfDoc);
    KEYBINDING(SelectAll, SelectAll);

#undef NARG
#undef NARG_SELECT
#undef NARG_ENUM
#undef KEYBINDING
#undef KEYBINDING_DISPATCH
#undef KEYBINDING_IMPL
#undef KEYBINDING_1
#undef KEYBINDING_2
#undef DEFAULT_KEYBINDING
}

bool TextInputCommandManager::insert_or_update(QKeySequence key, TextInputCommand cmd) {
    if (conflicts(key)) { return false; }
    key_to_cmd_[key] = cmd;
    cmd_to_key_[cmd] = key;
    return true;
}

bool TextInputCommandManager::conflicts(QKeySequence key) const {
    return key_to_cmd_.contains(key);
}

std::optional<QKeySequence> TextInputCommandManager::keybindings(TextInputCommand cmd) const {
    if (!cmd_to_key_.contains(cmd)) { return std::nullopt; }
    return {cmd_to_key_[cmd]};
}

void TextInputCommandManager::clear() {
    key_to_cmd_.clear();
    cmd_to_key_.clear();
}

TextInputCommand TextInputCommandManager::match(QKeyEvent *e) const {
    const auto modifiers = e->modifiers() & ~Qt::KeypadModifier;
    const auto key       = QKeyCombination::fromCombined(modifiers | e->key());
    if (key_to_cmd_.contains(key)) { return key_to_cmd_[key]; }
    if (is_printable_char(key)) { return TextInputCommand::InsertPrintable; }
    return TextInputCommand::Reject;
}

bool TextInputCommandManager::is_printable_char(QKeyCombination key) {
    const auto modifiers = key.keyboardModifiers() & ~Qt::KeypadModifier;
    if (modifiers & ~Qt::ShiftModifier) { return false; }
    if (key.key() >= Qt::Key_Space && key.key() <= Qt::Key_AsciiTilde) { return true; }
    const auto code = modifiers | static_cast<int>(key.key());
    if (code == Qt::Key_Tab) { return true; }
    if (code == Qt::Key_Enter || code == Qt::Key_Return) { return true; }
    return false;
}

QChar TextInputCommandManager::translate_printable_char(QKeyEvent *e) {
    Q_ASSERT(is_printable_char(e->keyCombination()));
    const auto code = e->key();
    if (code == Qt::Key_Tab) { return '\t'; }
    if (code == Qt::Key_Enter || code == Qt::Key_Return) { return '\n'; }
    return e->text().at(0);
}

GeneralTextInputCommandManager::GeneralTextInputCommandManager(const TextViewEngine &engine)
    : engine_(engine) {}

TextInputCommand GeneralTextInputCommandManager::match(QKeyEvent *e) const {
    const auto cmd = TextInputCommandManager::match(e);
    switch (cmd) {
        case TextInputCommand::MoveToStartOfLine: {
            if (engine_.cursor.col == 0) { return TextInputCommand::MoveToStartOfBlock; }
        } break;
        case TextInputCommand::MoveToEndOfLine: {
            const auto &cursor = engine_.cursor;
            const auto  block  = engine_.current_block();
            const auto  len    = block->len_of_line(cursor.row);
            if (cursor.col == len) { return TextInputCommand::MoveToEndOfBlock; }
        } break;
        case TextInputCommand::SelectToStartOfLine: {
            if (engine_.cursor.col == 0) { return TextInputCommand::SelectToStartOfBlock; }
        } break;
        case TextInputCommand::SelectToEndOfLine: {
            const auto &cursor = engine_.cursor;
            const auto  block  = engine_.current_block();
            const auto  len    = block->len_of_line(cursor.row);
            if (cursor.col == len) { return TextInputCommand::SelectToEndOfBlock; }
        } break;
        case TextInputCommand::InsertPrintable: {
            if (const auto c = translate_printable_char(e); c == '\n') {
                return TextInputCommand::InsertNewLine;
            } else if (c == '\t') {
                return TextInputCommand::InsertTab;
            }
        } break;
        default: {
        } break;
    }
    return cmd;
}

}; // namespace jwrite
