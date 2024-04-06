#pragma once

#include <QFontMetrics>
#include <QVector>
#include <QString>
#include <QStringView>
#include <QList>
#include <QDebug>

namespace jwrite {

struct TextBlock;
struct TextViewEngine;

struct TextLine {
    TextBlock *parent;
    int        line_nr;
    int        endp_offset;
    int        cached_text_width;
    int        cached_mean_width;

    void        markAsDirty() const;
    QStringView text() const;
    int         textOffset() const;
    double      charSpacing() const;
    bool        isFirstLine() const;
};

struct TextBlock {
    TextViewEngine *parent;
    const QString  *text_ref;
    int             text_pos;

    QVector<TextLine> lines;

    int dirty_line_nr;

    void            reset(const QString *ref, int pos);
    void            release();
    void            markAsDirty(int line_nr);
    bool            isDirty() const;
    void            joinDirtyLines();
    int             textLength() const;
    int             offsetOfLine(int index) const;
    int             lengthOfLine(int index) const;
    TextLine       &currentLine();
    const TextLine &currentLine() const;
    QStringView     textOfLine(int index) const;
    QStringView     text() const;
    void            squeezeAndExtendLastLine(int length);
    void            render();
};

struct TextViewEngine {
    constexpr static QChar SAMPLE_CHAR = QChar(U'\u3000');

    QFontMetrics fm;
    int          standard_char_width;
    int          max_width;

    int                  active_block_index;
    QVector<TextBlock *> active_blocks;

    int    line_height;
    double block_spacing;
    double line_spacing_ratio;

    struct CursorPosition {
        int pos;
        int row;
        int col;

        void reset();
    } cursor;

    int            text_ref_origin;
    const QString *text_ref;

    bool           preedit;
    CursorPosition saved_cursor;
    int            saved_text_pos;
    int            saved_text_length;
    const QString *preedit_text_ref;

    bool dirty;

    QList<TextBlock *> block_pool;

    TextViewEngine(const QFontMetrics &metrics, int width);

    void             reset(const QFontMetrics &metrics, int width);
    TextBlock       *allocBlock();
    bool             isEmpty() const;
    bool             isDirty() const;
    bool             isCursorAvailable() const;
    void             markAsDirty();
    TextLine        &currentLine();
    const TextLine  &currentLine() const;
    TextBlock       *currentBlock();
    const TextBlock *currentBlock() const;
    void             resetMaxWidth(int width);
    void             resetBlockSpacing(double spacing);
    void             resetLineSpacing(double ratio);
    void             resetFontMetrics(const QFontMetrics &metrics);
    void             syncCursorRowCol();
    void             render();

    //! TODO: promote unsafe method into the safe one
    void setTextRefUnsafe(const QString *ref, int ref_origin);
    void insertBlock(int index);
    void breakBlockAtCursorPos();
    void commitInsertion(int text_length);
    int  commitDeletion(int times, int &deleted);
    int  commitMovement(int offset, bool *moved);
    void beginPreEdit(QString &ref);
    void updatePreEditText(int text_length);
    void commitPreEdit();

    static int boundingTextLength(const QFontMetrics &fm, QStringView text, int &width);
};

}; // namespace jwrite

QDebug operator<<(QDebug dbg, const jwrite::TextViewEngine::CursorPosition &pos);
