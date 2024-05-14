#include "Helper.h"
#include <gtest/gtest.h>

TEST(TextEdit, InitState) {
    auto e = TextViewEngine::get_single_line_engine();
    e.check_cursor(0, 0, 0, 0);
}

TEST(TextEdit, SingleBlockSeqInsert) {
    auto e = TextViewEngine::get_single_line_engine();

    int  acc   = 0;
    auto block = e.current_block();
    for (int i = 0; i < 256; ++i) {
        const int  len      = gen_random_int(0, 32);
        const auto inserted = gen_random_str(len);
        e.insert(inserted);
        acc += len;
        ASSERT_EQ(e.text, block->text());
        e.check_cursor(0, acc, 0, acc);
    }
}

TEST(TextEdit, SingleBlockRandomInsert) {
    auto e = TextViewEngine::get_single_line_engine();

    auto block = e.current_block();
    for (int i = 0; i < 256; ++i) {
        const int  pos      = gen_random_int(0, block->text_len() + 1);
        const int  len      = gen_random_int(0, 32);
        const auto inserted = gen_random_str(len);
        e.cursor.pos        = pos;
        e.cursor.col        = pos;
        e.insert(inserted);
        ASSERT_EQ(e.text, block->text());
        e.check_cursor(0, pos + len, 0, pos + len);
    }
}

TEST(TextEdit, MultiBlockInsert) {
    auto e = TextViewEngine::get_single_line_engine();
    e.gen_blocks(32);
    //! TODO: rt.
}

TEST(TextEdit, PreEditBlockInsert) {
    auto e = TextViewEngine::get_single_line_engine();
    e.gen_blocks(32);
    //! TODO: rt.
}

TEST(TextEdit, BreakLine) {
    auto e = TextViewEngine::get_single_line_engine();

    const int len = 256;
    e.insert(gen_random_str(256));
    ASSERT_EQ(e.active_blocks.size(), 1);

    e.break_block_at_cursor_pos();
    e.check_cursor(1, 0, 0, 0);
    ASSERT_EQ(e.active_blocks.size(), 2);
    ASSERT_EQ(e.active_blocks[0]->text(), e.text);
    ASSERT_TRUE(e.active_blocks[1]->text().isEmpty());

    e.reset_cursor_unsafe(0, 0, 0, 0);
    e.break_block_at_cursor_pos();
    e.check_cursor(1, 0, 0, 0);
    ASSERT_EQ(e.active_blocks.size(), 3);
    ASSERT_TRUE(e.active_blocks[0]->text().isEmpty());
    ASSERT_EQ(e.active_blocks[1]->text(), e.text);
    ASSERT_TRUE(e.active_blocks[2]->text().isEmpty());

    const int pos = gen_random_int(1, e.active_blocks[1]->text_len());
    e.reset_cursor_unsafe(1, pos, 0, pos);
    e.break_block_at_cursor_pos();
    e.check_cursor(2, 0, 0, 0);
    ASSERT_EQ(e.active_blocks.size(), 4);
    ASSERT_TRUE(e.active_blocks[0]->text().isEmpty());
    ASSERT_EQ(e.active_blocks[1]->text(), e.text.mid(0, pos));
    ASSERT_EQ(e.active_blocks[2]->text(), e.text.mid(pos));
    ASSERT_TRUE(e.active_blocks[3]->text().isEmpty());
}
