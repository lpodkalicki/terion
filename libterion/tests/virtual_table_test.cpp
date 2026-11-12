/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <gtest/gtest.h>
#include <terion/terion.h>

using namespace terion;

TEST(VirtualTable, OnlyPaintsVisibleDataRows)
{
    int row_reads = 0;
    Reconciler tree;
    tree.update(VirtualTable({
        .columns = {{.title = "ID",.width = 8},{.title = "Name",.width = 20}},
        .row_count = 100000,
        .row_at = [&](int index) {
            ++row_reads;
            return TableRow{.key = "",.cells = {std::to_string(index),"Entry "+std::to_string(index)}};
        },
        .layout = {.width = 30,.height = 6}
    }));
    ASSERT_NE(tree.root(),nullptr);
    ASSERT_EQ(tree.root()->children().size(),2U);
    EXPECT_EQ(row_reads,0);
    tree.root()->layout({.maximum = {.width = 30,.height = 6}});
    ScreenBuffer screen({.width = 30,.height = 6});
    Painter painter(screen);
    tree.root()->paint(painter);
    EXPECT_EQ(row_reads,5);
    EXPECT_EQ(tree.root()->children()[1]->children().size(),0U);
    EXPECT_TRUE(tree.root()->children()[1]->scrollBy(500));
    row_reads = 0;
    tree.root()->paint(painter);
    EXPECT_EQ(row_reads,5);
}

TEST(VirtualTable, HeaderSortAndRowSelection)
{
    int selected = -1;
    int sort_col = -1;
    bool sort_asc = false;
    Reconciler tree;
    tree.update(VirtualTable({
        .columns = {{.title = "ID",.width = 8}},
        .row_count = 50,
        .row_at = [](int index) { return TableRow{.key = "",.cells = {std::to_string(index)}}; },
        .on_select = [&](int index) { selected = index; },
        .on_sort = [&](int col,bool ascending) { sort_col = col; sort_asc = ascending; },
        .layout = {.width = 10,.height = 4}
    }));
    tree.root()->layout({.maximum = {.width = 10,.height = 4}});
    ASSERT_EQ(tree.root()->children().size(),2U);
    EXPECT_TRUE(tree.root()->children()[1]->handleVirtualListClick({.x = 1,.y = 2}));
    EXPECT_EQ(selected,1);
    // Sorting is explicit and does not scan all rows.
    ASSERT_EQ(tree.root()->children()[0]->children().size(),1U);
    const auto& sort_action = tree.root()->children()[0]->children()[0]->onClick();
    ASSERT_TRUE(static_cast<bool>(sort_action));
    sort_action();
    EXPECT_EQ(sort_col,0);
    EXPECT_TRUE(sort_asc);
}

TEST(VirtualTable, DoesNotReadDisabledRowOnPaintForOtherRows)
{
    int reads = 0;
    Reconciler tree;
    tree.update(VirtualTable({
        .columns = {{.title = "Label",.width = 16}},
        .row_count = 1000000,
        .row_at = [&](int index) { ++reads; return TableRow{.key = "",.cells = {std::to_string(index)}}; },
        .layout = {.width = 18,.height = 3}
    }));
    tree.root()->layout({.maximum = {.width = 18,.height = 3}});
    ScreenBuffer screen({.width = 18,.height = 3});
    Painter painter(screen);
    tree.root()->paint(painter);
    EXPECT_EQ(reads,2);
}
