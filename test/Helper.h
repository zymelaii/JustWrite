#include <TextViewEngine.h>
#include <QString>
#include <QApplication>
#include <gtest/gtest.h>

int     genRandomInt(int lo, int hi);
QString genRandomString(int length);

struct TextViewEngine : public jwrite::TextViewEngine {
    TextViewEngine(int width)
        : jwrite::TextViewEngine(QFontMetrics(QApplication::font()), width) {
        setTextRefUnsafe(&text, 0);
        insertBlock(0);
        active_block_index = 0;
    }

    void genBlocks(int n) {
        const auto old_cursor      = cursor;
        const int  old_block_index = active_block_index;
        for (int i = 0; i < n; ++i) {
            insertBlock(active_blocks.size());
            resetCursorUnsafe(active_blocks.size() - 1, 0, 0, 0);
            insert(genRandomString(genRandomInt(0, 256)));
        }
        active_block_index = old_block_index;
        cursor             = old_cursor;
    }

    void insert(const QString &inserted) {
        auto block = currentBlock();
        text.insert(block->text_pos + cursor.pos, inserted);
        commitInsertion(inserted.length());
    }

    static TextViewEngine singleLineEngine() {
        return TextViewEngine(std::numeric_limits<int>::max());
    }

    void checkCursor(int block_index, int pos, int row, int col) const {
        ASSERT_EQ(active_block_index, block_index);
        ASSERT_EQ(cursor.pos, pos);
        ASSERT_EQ(cursor.row, row);
        ASSERT_EQ(cursor.col, col);
    }

    void resetCursorUnsafe(int block_index, int pos, int row, int col) {
        active_block_index = block_index;
        cursor.pos         = pos;
        cursor.row         = row;
        cursor.col         = col;
    }

    QString text;
};
