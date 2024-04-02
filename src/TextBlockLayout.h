#pragma once

#include <QStringView>
#include <QVector>
#include <QFontMetrics>
#include <QDebug>

struct TextLineLayout {
    int  text_endp;
    int  offset;
    int  text_width;
    int  mean_width;
    bool dirty;

    void reset(int pos) {
        text_endp = pos;
        offset    = 0;
        dirty     = true;
    }
};

struct TextBlockLayout {
    QVector<TextLineLayout> lines;
    QStringView             text_ref;
    int                     text_pos;
    int                     cursor_pos;
    int                     cursor_row;
    int                     cursor_col;
    int                     max_width;

    void reset(QStringView text, int start_pos);
    void setMaxWidth(int width);
    void checkOrUpdateLeadingSpace(const QFontMetrics &fm);

    void shiftOrigin(int offset);
    void quickInsert(qsizetype size);

    void foldInto(int index);

    void render(const QFontMetrics &fm);

    QStringView line(int index) const;

    TextBlockLayout split();

    TextBlockLayout &sync(QStringView source) {
        text_ref = source;
        return *this;
    }

    bool isEmpty() const {
        return lines[0].text_endp == text_pos;
    }

    static int boundingTextLength(const QFontMetrics &fm, QStringView text, int &width);
};

QDebug operator<<(QDebug dbg, const TextBlockLayout &block);
