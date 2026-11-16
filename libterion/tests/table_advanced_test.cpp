/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <gtest/gtest.h>
#include <terion/terion.h>
#include <terion/components/table_view_detail.h>

using namespace terion;

TEST(TableAdvanced, VirtualTableRespectsRgbForegroundBackgroundAndBold)
{
    const Color fg = Color::rgb(21,177,95);
    const Color bg = Color::rgb(24,30,51);
    Reconciler tree;
    tree.update(VirtualTable({.columns = {{.title = "Status",.width = 12}},
        .row_count = 1,
        .row_at = [](int) { return TableRow{.key = "a",.cells = {"Online"}}; },
        .format_cell = [fg,bg](const TableRow&,std::size_t) {
            return TableCellFormat{.text = "Online",.foreground = fg,.background = bg,.bold = true};
        },
        .layout = {.width = 14,.height = 3},.show_scrollbar = false}));
    tree.root()->layout({.maximum = {.width = 14,.height = 3}});
    ScreenBuffer screen({.width = 14,.height = 3});
    Painter painter(screen);
    tree.root()->paint(painter);
    EXPECT_EQ(screen.at(0,1).grapheme,"O");
    EXPECT_EQ(screen.at(0,1).style.foreground,fg);
    EXPECT_EQ(screen.at(0,1).style.background,bg);
    EXPECT_TRUE(screen.at(0,1).style.bold);
    EXPECT_EQ(screen.at(8,1).style.background,bg);
}

TEST(TableAdvanced, VirtualTableSelectionUsesStableKeysAndOnlyVisibleRows)
{
    int reads = 0;
    bool reverse = false;
    std::string selected;
    const auto make_table = [&] {
        return VirtualTable({.columns = {{.title = "ID",.width = 12}},
            .row_count = 100000,
            .row_at = [&](int index) {
                ++reads;
                const int id = reverse ? 99999-index : index;
                return TableRow{.key = "id-"+std::to_string(id),.cells = {std::to_string(id)}};
            },
            .row_key_at = [&](int index) {
                return "id-"+std::to_string(reverse ? 99999-index : index);
            },
            .selected_key = "id-99998",
            .on_selection_key_change = [&](std::string key) { selected = std::move(key); },
            .layout = {.width = 16,.height = 4}});
    };
    Reconciler tree;
    tree.update(make_table());
    tree.root()->layout({.maximum = {.width = 16,.height = 4}});
    ScreenBuffer screen({.width = 16,.height = 4});
    Painter painter(screen);
    tree.root()->paint(painter);
    EXPECT_EQ(reads,3); // No global key scan, only 3 painted rows.
    reverse = true;
    reads = 0;
    tree.update(make_table());
    tree.root()->layout({.maximum = {.width = 16,.height = 4}});
    tree.root()->paint(painter);
    EXPECT_EQ(reads,3);
    ASSERT_TRUE(tree.root()->children()[1]->handleVirtualListClick({.x = 0,.y = 2}));
    EXPECT_EQ(selected,"id-99998");
}

TEST(TableAdvanced, VirtualTableHorizontalScrollClipsUnicodeWithoutSplitting)
{
    const std::string original = "ab👍🏽cd";
    const auto left = detail::sliceColumns(original,0,4);
    const auto middle = detail::sliceColumns(original,3,3);
    EXPECT_EQ(displayWidth(left),4);
    EXPECT_EQ(displayWidth(middle),3);
    EXPECT_EQ(middle," cd"); // Incomplete emoji is replaced by one blank column.
    int offset = -1;
    Reconciler tree;
    tree.update(VirtualTable({.columns = {{.title = "First",.width = 12},
            {.title = "Second",.width = 12}},
        .row_count = 1,
        .row_at = [](int) { return TableRow{.key = "row",.cells = {"ABCDEFGHIJKLMNOP","XYZ"}}; },
        .horizontal_offset = 8,
        .on_horizontal_scroll = [&](int next) { offset = next; },
        .layout = {.width = 14,.height = 3},.show_scrollbar = false}));
    tree.root()->layout({.maximum = {.width = 14,.height = 3}});
    ScreenBuffer screen({.width = 14,.height = 3});
    Painter painter(screen);
    tree.root()->paint(painter);
    EXPECT_EQ(screen.at(0,1).grapheme,"I");
    const auto& header = tree.root()->children()[0]->children();
    ASSERT_GE(header.size(),2U);
    const auto& next = header.back()->onClick();
    ASSERT_TRUE(static_cast<bool>(next));
    next();
    EXPECT_GT(offset,8);
}

TEST(TableAdvanced, TableHorizontalScrollAndHeaderClickUseSameColumns)
{
    Reconciler tree;
    tree.update(Table({.columns = {{"First",12},{"Second",12},{"Third",12}},
        .rows = {{"a",{"AAAAA","BBBBB","CCCCC"}}},
        .layout = {.width = 17,.height = 4}}));
    tree.root()->layout({.maximum = {.width = 17,.height = 4}});
    auto* table = tree.root();
    EXPECT_EQ(table->table_horizontal_offset(),0);
    EXPECT_TRUE(table->handleTableKey({.key = Key::Right,.shift = true}));
    EXPECT_EQ(table->table_horizontal_offset(),3);
    EXPECT_TRUE(table->handleTableKey({.key = Key::End,.shift = true}));
    const int offset = table->table_horizontal_offset();
    EXPECT_GT(offset,3);
    EXPECT_TRUE(table->handleTableClick({.x = 8,.y = 0}));
    EXPECT_EQ(table->table_sort_column(),2);
    EXPECT_TRUE(table->handleTableKey({.key = Key::Home,.shift = true}));
    EXPECT_EQ(table->table_horizontal_offset(),0);
}

TEST(TableAdvanced, VirtualTableDisabledRowsDoNotSelectByKey)
{
    std::string selected;
    Reconciler tree;
    tree.update(VirtualTable({.columns = {{.title = "Name",.width = 10}},
        .row_count = 2,
        .row_at = [](int index) { return TableRow{.key = "key-"+std::to_string(index),
            .cells = {"Text"},.disabled = index == 1}; },
        .on_selection_key_change = [&](std::string key) { selected = std::move(key); },
        .layout = {.width = 12,.height = 3}}));
    tree.root()->layout({.maximum = {.width = 12,.height = 3}});
    EXPECT_TRUE(tree.root()->children()[1]->handleVirtualListClick({.x = 0,.y = 1}));
    EXPECT_EQ(selected,"key-0");
    EXPECT_TRUE(tree.root()->children()[1]->handleVirtualListClick({.x = 0,.y = 2}));
    EXPECT_EQ(selected,"key-0");
}

TEST(TableAdvanced, VirtualTableKeyboardNavigationStartsFromVisibleSelectedKey)
{
    std::string selected = "key-2";
    const auto render = [&] {
        return VirtualTable({.columns = {{.title = "Value",.width = 12}},
            .row_count = 100000,
            .row_at = [](int index) { return TableRow{.key = "key-"+std::to_string(index),
                .cells = {std::to_string(index)}}; },
            .row_key_at = [](int index) { return "key-"+std::to_string(index); },
            .selected_key = selected,
            .on_selection_key_change = [&](std::string key) { selected = std::move(key); },
            .layout = {.width = 14,.height = 5}});
    };
    Reconciler tree;
    tree.update(render());
    tree.root()->layout({.maximum = {.width = 14,.height = 5}});
    EXPECT_TRUE(tree.root()->children()[1]->handleVirtualListKey({.key = Key::Down}));
    EXPECT_EQ(selected,"key-3");
    tree.update(render());
    tree.root()->layout({.maximum = {.width = 14,.height = 5}});
    EXPECT_TRUE(tree.root()->children()[1]->handleVirtualListKey({.key = Key::Up}));
    EXPECT_EQ(selected,"key-2");
}
