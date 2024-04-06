#include "Helper.h"
#include <gtest/gtest.h>

TEST(TextEdit, InitState) {
    auto e = TextViewEngine::singleLineEngine();
    e.checkCursor(0, 0, 0, 0);
}

TEST(TextEdit, SingleBlockSeqInsert) {
    auto e = TextViewEngine::singleLineEngine();

    int  acc   = 0;
    auto block = e.currentBlock();
    for (int i = 0; i < 256; ++i) {
        const int  len      = genRandomInt(0, 32);
        const auto inserted = genRandomString(len);
        e.insert(inserted);
        acc += len;
        ASSERT_EQ(e.text, block->text());
        e.checkCursor(0, acc, 0, acc);
    }
}

TEST(TextEdit, SingleBlockRandomInsert) {
    auto e = TextViewEngine::singleLineEngine();

    auto block = e.currentBlock();
    for (int i = 0; i < 256; ++i) {
        const int  pos      = genRandomInt(0, block->textLength() + 1);
        const int  len      = genRandomInt(0, 32);
        const auto inserted = genRandomString(len);
        e.cursor.pos        = pos;
        e.cursor.col        = pos;
        e.insert(inserted);
        ASSERT_EQ(e.text, block->text());
        e.checkCursor(0, pos + len, 0, pos + len);
    }
}

TEST(TextEdit, MultiBlockInsert) {
    auto e = TextViewEngine::singleLineEngine();
    e.genBlocks(32);
    //! TODO: rt.
}

TEST(TextEdit, PreEditBlockInsert) {
    auto e = TextViewEngine::singleLineEngine();
    e.genBlocks(32);
    //! TODO: rt.
}

TEST(TextEdit, BreakLine) {
    auto e = TextViewEngine::singleLineEngine();

    const int len = 256;
    e.insert(genRandomString(256));
    ASSERT_EQ(e.active_blocks.size(), 1);

    e.breakBlockAtCursorPos();
    e.checkCursor(1, 0, 0, 0);
    ASSERT_EQ(e.active_blocks.size(), 2);
    ASSERT_EQ(e.active_blocks[0]->text(), e.text);
    ASSERT_TRUE(e.active_blocks[1]->text().isEmpty());

    e.resetCursorUnsafe(0, 0, 0, 0);
    e.breakBlockAtCursorPos();
    e.checkCursor(1, 0, 0, 0);
    ASSERT_EQ(e.active_blocks.size(), 3);
    ASSERT_TRUE(e.active_blocks[0]->text().isEmpty());
    ASSERT_EQ(e.active_blocks[1]->text(), e.text);
    ASSERT_TRUE(e.active_blocks[2]->text().isEmpty());

    const int pos = genRandomInt(1, e.active_blocks[1]->textLength());
    e.resetCursorUnsafe(1, pos, 0, pos);
    e.breakBlockAtCursorPos();
    e.checkCursor(2, 0, 0, 0);
    ASSERT_EQ(e.active_blocks.size(), 4);
    ASSERT_TRUE(e.active_blocks[0]->text().isEmpty());
    ASSERT_EQ(e.active_blocks[1]->text(), e.text.mid(0, pos));
    ASSERT_EQ(e.active_blocks[2]->text(), e.text.mid(pos));
    ASSERT_TRUE(e.active_blocks[3]->text().isEmpty());
}
