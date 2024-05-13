#pragma once

#include <QKeyEvent>
#include <QMap>
#include <optional>
#include "TextViewEngine.h"

enum class TextInputCommand {
    Reject,
    InsertPrintable,
    InsertTab,
    InsertNewLine,
    InsertBeforeBlock,
    InsertAfterBlock,
    Cancel,
    Undo,
    Redo,
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
    SelectPrevChar,
    SelectNextChar,
    SelectPrevWord,
    SelectNextWord,
    SelectToStartOfLine,
    SelectToEndOfLine,
    SelectToStartOfBlock,
    SelectToEndOfBlock,
    SelectBlock,
    SelectPrevPage,
    SelectNextPage,
    SelectToStartOfDoc,
    SelectToEndOfDoc,
    SelectAll,
};

class TextInputCommandManager {
public:
    void                        loadDefaultMappings();
    bool                        insertOrUpdateKeyBinding(QKeySequence key, TextInputCommand cmd);
    bool                        conflicts(QKeySequence key) const;
    std::optional<QKeySequence> keyBinding(TextInputCommand cmd) const;
    void                        clear();

    virtual TextInputCommand match(QKeyEvent *e) const;

    static bool  isPrintableChar(QKeyCombination e);
    static QChar translatePrintableChar(QKeyEvent *e);

private:
    QMap<QKeySequence, TextInputCommand> key_to_cmd_;
    QMap<TextInputCommand, QKeySequence> cmd_to_key_;
};

namespace jwrite {

class TextInputCommandManager : public ::TextInputCommandManager {
public:
    TextInputCommandManager(const TextViewEngine &engine);
    TextInputCommand match(QKeyEvent *e) const override;

private:
    const TextViewEngine &engine_;
};

}; // namespace jwrite
