#pragma once

#include <QKeyEvent>

enum class TextInputType {
    Reject,
    Printable,
    Copy,
    Cut,
    Paste,
    ScrollUp,
    ScrollDown,
    MoveToPrevChar,
    MoveToNextChar,
    MoveToPrevWord,
    MoveToNextWord,
    MoveToPrevLine,
    MoveToNextLine,
    MoveToStartOfLine,
    MoveToEndOfLine,
    MoveToStartOfBlock,
    MoveToEndOfBlock,
    MoveToStartOfDocument,
    MoveToEndOfDocument,
    MoveToPrevPage,
    MoveToNextPage,
    MoveToPrevBlock,
    MoveToNextBlock,
    DeletePrevChar,
    DeleteNextChar,
    DeletePrevWord,
    DeleteNextWord,
    DeleteToStartOfLine,
    DeleteToEndOfLine,
    DeleteToStartOfBlock,
    DeleteToEndOfBlock,
    DeleteToStartOfDocument,
    DeleteToEndOfDocument,
    InsertBeforeBlock,
    InsertAfterBlock,
};

inline bool isPrintableChar(QKeyEvent* e) {
    if (e->modifiers() & ~Qt::ShiftModifier) { return false; }
    if (e->key() >= Qt::Key_Space && e->key() <= Qt::Key_AsciiTilde) { return true; }
    const auto code = e->key() | e->modifiers();
    if (code == Qt::Key_Tab) { return true; }
    if (code == Qt::Key_Enter || code == Qt::Key_Return) { return true; }
    return false;
}

inline QChar translatePrintableChar(QKeyEvent* e) {
    Q_ASSERT(isPrintableChar(e));
    if (e->key() == Qt::Key_Tab) { return '\t'; }
    if (e->key() == Qt::Key_Enter || e->key() == Qt::Key_Return) { return '\n'; }
    return e->text().at(0);
}

inline TextInputType textInputFilter(QKeyEvent* e) {
    if (isPrintableChar(e)) { return TextInputType::Printable; }

    enum KeyCombination {
        ScrollUp            = Qt::ControlModifier | Qt::Key_Up,
        ScrollDown          = Qt::ControlModifier | Qt::Key_Down,
        MoveToPrevBlock     = Qt::ShiftModifier | Qt::AltModifier | Qt::Key_Return,
        MoveToNextBlock     = Qt::AltModifier | Qt::Key_Return,
        DeleteToStartOfLine = Qt::ControlModifier | Qt::Key_U,
        DeleteToEndOfLine   = Qt::ControlModifier | Qt::Key_K,
        InsertBeforeBlock   = Qt::ShiftModifier | Qt::ControlModifier | Qt::Key_Return,
        InsertAfterBlock    = Qt::ControlModifier | Qt::Key_Return,
        DeletePrevChar      = Qt::Key_Backspace,
    };

    const auto code = e->key() | e->modifiers();

    if (e->matches(QKeySequence::Copy)) { return TextInputType::Copy; }
    if (e->matches(QKeySequence::Cut)) { return TextInputType::Cut; }
    if (e->matches(QKeySequence::Paste)) { return TextInputType::Paste; }
    if (code == KeyCombination::ScrollUp) { return TextInputType::ScrollUp; }
    if (code == KeyCombination::ScrollDown) { return TextInputType::ScrollDown; }
    if (e->matches(QKeySequence::MoveToPreviousChar)) { return TextInputType::MoveToPrevChar; }
    if (e->matches(QKeySequence::MoveToNextChar)) { return TextInputType::MoveToNextChar; }
    if (e->matches(QKeySequence::MoveToPreviousWord)) { return TextInputType::MoveToPrevWord; }
    if (e->matches(QKeySequence::MoveToNextWord)) { return TextInputType::MoveToNextWord; }
    if (e->matches(QKeySequence::MoveToPreviousLine)) { return TextInputType::MoveToPrevLine; }
    if (e->matches(QKeySequence::MoveToNextLine)) { return TextInputType::MoveToNextLine; }
    if (e->matches(QKeySequence::MoveToStartOfLine)) { return TextInputType::MoveToStartOfLine; }
    if (e->matches(QKeySequence::MoveToEndOfLine)) { return TextInputType::MoveToEndOfLine; }
    if (e->matches(QKeySequence::MoveToStartOfBlock)) { return TextInputType::MoveToStartOfBlock; }
    if (e->matches(QKeySequence::MoveToEndOfBlock)) { return TextInputType::MoveToEndOfBlock; }
    if (e->matches(QKeySequence::MoveToStartOfDocument)) {
        return TextInputType::MoveToStartOfDocument;
    }
    if (e->matches(QKeySequence::MoveToEndOfDocument)) {
        return TextInputType::MoveToEndOfDocument;
    }
    if (e->matches(QKeySequence::MoveToPreviousPage)) { return TextInputType::MoveToPrevPage; }
    if (e->matches(QKeySequence::MoveToNextPage)) { return TextInputType::MoveToNextPage; }
    if (code == KeyCombination::MoveToPrevBlock) { return TextInputType::MoveToPrevBlock; }
    if (code == KeyCombination::MoveToNextBlock) { return TextInputType::MoveToNextBlock; }
    if (code == DeletePrevChar) { return TextInputType::DeletePrevChar; }
    if (e->matches(QKeySequence::Delete)) { return TextInputType::DeleteNextChar; }
    if (e->matches(QKeySequence::DeleteStartOfWord)) { return TextInputType::DeletePrevWord; }
    if (e->matches(QKeySequence::DeleteEndOfWord)) { return TextInputType::DeleteNextWord; }
    if (code == KeyCombination::DeleteToStartOfLine) { return TextInputType::DeleteToStartOfLine; }
    if (code == KeyCombination::DeleteToEndOfLine) { return TextInputType::DeleteToEndOfLine; }
    if (false) { return TextInputType::DeleteToStartOfBlock; }
    if (false) { return TextInputType::DeleteToEndOfBlock; }
    if (false) { return TextInputType::DeleteToStartOfDocument; }
    if (false) { return TextInputType::DeleteToEndOfDocument; }
    if (code == KeyCombination::InsertBeforeBlock) { return TextInputType::InsertBeforeBlock; }
    if (code == KeyCombination::InsertAfterBlock) { return TextInputType::InsertAfterBlock; }

    return TextInputType::Reject;
}
