#include <jwrite/TextViewEngine.h>
#include <QString>
#include <QApplication>
#include <gtest/gtest.h>

int     gen_random_int(int lo, int hi);
QString gen_random_str(int length);

struct TextViewEngine : public jwrite::TextViewEngine {
    TextViewEngine(int width)
        : jwrite::TextViewEngine(QFontMetrics(QApplication::font()), width) {
        set_text_ref_unsafe(&text, 0);
        insert_block(0);
        active_block_index = 0;
    }

    void gen_blocks(int n) {
        const auto old_cursor      = cursor;
        const int  old_block_index = active_block_index;
        for (int i = 0; i < n; ++i) {
            insert_block(active_blocks.size());
            reset_cursor_unsafe(active_blocks.size() - 1, 0, 0, 0);
            insert(gen_random_str(gen_random_int(0, 256)));
        }
        active_block_index = old_block_index;
        cursor             = old_cursor;
    }

    void insert(const QString &inserted) {
        auto block = current_block();
        text.insert(block->text_pos + cursor.pos, inserted);
        commit_insertion(inserted.length());
    }

    static TextViewEngine get_single_line_engine() {
        return TextViewEngine(std::numeric_limits<int>::max());
    }

    void check_cursor(int block_index, int pos, int row, int col) const {
        ASSERT_EQ(active_block_index, block_index);
        ASSERT_EQ(cursor.pos, pos);
        ASSERT_EQ(cursor.row, row);
        ASSERT_EQ(cursor.col, col);
    }

    void reset_cursor_unsafe(int block_index, int pos, int row, int col) {
        active_block_index = block_index;
        cursor.pos         = pos;
        cursor.row         = row;
        cursor.col         = col;
    }

    QString text;
};
