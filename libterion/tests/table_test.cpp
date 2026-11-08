/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <gtest/gtest.h>
#include <terion/terion.h>

using namespace terion;

TEST(Table, KeyboardSelectionSkipsDisabledRows)
{
    std::string selected;
    Reconciler tree;
    tree.update(Table({.columns = {{"Name"}},.rows = {
        {"one",{"One"}},{"two",{"Two"},true},{"three",{"Three"}}},
        .value = "one",.onChange = [&](std::string value) { selected = std::move(value); }}));
    tree.root()->layout({.maximum = {.width = 40,.height = 10}});
    EXPECT_TRUE(tree.root()->handleTableKey({.key = Key::Down}));
    EXPECT_EQ(selected,"three");
    EXPECT_TRUE(tree.root()->handleTableKey({.key = Key::End}));
    EXPECT_EQ(selected,"three");
}

TEST(Table, MouseSelectionRespectsHeaderAndRowPosition)
{
    std::string selected;
    Reconciler tree;
    tree.update(Table({.columns = {{"Name"}},.rows = {
        {"one",{"One"}},{"two",{"Two"}}},.value = "one",
        .onChange = [&](std::string value) { selected = std::move(value); }}));
    tree.root()->layout({.maximum = {.width = 40,.height = 10}});
    EXPECT_FALSE(tree.root()->handleTableClick({.x = 1,.y = 0}));
    EXPECT_TRUE(tree.root()->handleTableClick({.x = 1,.y = 3}));
    EXPECT_EQ(selected,"two");
}

TEST(Table, PaintsHeaderAndClipsRowsToViewport)
{
    Reconciler tree;
    tree.update(Table({.columns = {{"Name",10}},.rows = {
        {"one",{"Alpha"}},{"two",{"Beta"}},{"three",{"Gamma"}}},
        .value = "one",.layout = {.height = 3}}));
    tree.root()->layout({.maximum = {.width = 20,.height = 6}});
    ScreenBuffer screen({.width = 20,.height = 6});
    Painter painter(screen);
    tree.root()->paint(painter);
    EXPECT_EQ(screen.at(1,0).grapheme,"N");
    EXPECT_EQ(screen.at(1,2).grapheme,"A");
    EXPECT_EQ(screen.at(1,3).grapheme," ");
}

TEST(Table, HeaderClickSortsRowsAndTogglesDirection)
{
    std::string selected;
    Reconciler tree;
    tree.update(Table({.columns = {{"Name",12}},.rows = {
        {"b",{"Bravo"}},{"a",{"Alpha"}},{"c",{"Charlie"}}},
        .value = "b",.onChange = [&](std::string key) { selected = std::move(key); }}));
    tree.root()->layout({.maximum = {.width = 25,.height = 8}});
    EXPECT_TRUE(tree.root()->handleTableClick({.x = 2,.y = 0}));
    EXPECT_EQ(tree.root()->tableSortColumn(),0);
    EXPECT_TRUE(tree.root()->tableSortAscending());
    EXPECT_EQ(tree.root()->tableOrder()[0],1U);
    EXPECT_TRUE(tree.root()->handleTableClick({.x = 2,.y = 0}));
    EXPECT_FALSE(tree.root()->tableSortAscending());
    EXPECT_EQ(tree.root()->tableOrder()[0],2U);
    EXPECT_TRUE(tree.root()->handleTableClick({.x = 2,.y = 2}));
    EXPECT_EQ(selected,"c");
}

TEST(Table, ScrollbarDragAndWheelShareScrollOffset)
{
    Reconciler tree;
    std::vector<TableRow> rows;
    for (int i = 0; i < 30; ++i)
        rows.push_back({.key = std::to_string(i),.cells = {"Row "+std::to_string(i)}});
    tree.update(Table({.columns = {{"Name",12}},.rows = std::move(rows),
        .layout = {.height = 8}}));
    tree.root()->layout({.maximum = {.width = 20,.height = 8}});
    auto* table = tree.root();
    const Rect track = table->scrollbarTrack();
    ASSERT_EQ(track.height,6);
    EXPECT_EQ(track.x,table->bounds().x+table->bounds().width-1);
    EXPECT_TRUE(table->scrollBy(8));
    EXPECT_GT(table->scrollbarThumb().y,track.y);
    int grab = 0;
    EXPECT_TRUE(table->scrollbarPress({.x = track.x,.y = track.y+track.height-1},grab));
    EXPECT_TRUE(table->scrollbarDrag(track.y+track.height-1,grab));
    EXPECT_EQ(table->scrollbarThumb().y+table->scrollbarThumb().height,
        track.y+track.height);
    EXPECT_TRUE(table->scrollBy(-1000));
    EXPECT_EQ(table->scrollbarThumb().y,track.y);
}

TEST(Table, FilterAffectsSortingSelectionAndScrollbar)
{
    std::string selected;
    Reconciler tree;
    tree.update(Table({.columns = {{"Name",12}},.rows = {
        {"a",{"Alpha"}},{"b",{"Bravo"}},{"c",{"Charlie"}},
        {"d",{"Delta"}},{"e",{"Echo"}}},
        .filter = [](const TableRow& row) { return row.key == "b" || row.key == "d"; },
        .value = "b",.layout = {.height = 5},
        .onChange = [&](std::string value) { selected = std::move(value); }}));
    tree.root()->layout({.maximum = {.width = 20,.height = 5}});
    const auto order = tree.root()->tableOrder();
    ASSERT_EQ(order.size(),2U);
    EXPECT_EQ(order[0],1U);
    EXPECT_EQ(order[1],3U);
    EXPECT_EQ(tree.root()->scrollbarTrack().height,0);
    EXPECT_TRUE(tree.root()->handleTableKey({.key = Key::Down}));
    EXPECT_EQ(selected,"d");
    tree.update(Table({.columns = {{"Name",12}},.rows = {
        {"a",{"Alpha"}},{"b",{"Bravo"}},{"c",{"Charlie"}},
        {"d",{"Delta"}},{"e",{"Echo"}}},
        .filter = [](const TableRow& row) { return row.key == "b" || row.key == "d"; },
        .value = selected,.layout = {.height = 5},
        .onChange = [&](std::string value) { selected = std::move(value); }}));
    tree.root()->layout({.maximum = {.width = 20,.height = 5}});
    EXPECT_TRUE(tree.root()->handleTableClick({.x = 2,.y = 2}));
    EXPECT_EQ(selected,"b");
}

TEST(Table, AlignsAndFormatsIndividualCells)
{
    Reconciler tree;
    const Color custom = Color::rgb(17,201,91);
    tree.update(Table({.columns = {{"Left",10,TableAlignment::Left},
            {"Center",10,TableAlignment::Center},{"Right",10,TableAlignment::Right}},
        .rows = {{"one",{"A","B","C"}}},
        .formatCell = [custom](const TableRow& row,std::size_t index) {
            return TableCellFormat{.text = row.cells[index],.foreground = custom,.bold = true};
        }}));
    tree.root()->layout({.maximum = {.width = 40,.height = 5}});
    ScreenBuffer screen({.width = 40,.height = 5});
    Painter painter(screen);
    tree.root()->paint(painter);
    EXPECT_EQ(screen.at(1,2).grapheme,"A");
    EXPECT_EQ(screen.at(16,2).grapheme,"B");
    EXPECT_EQ(screen.at(31,2).grapheme,"C");
    EXPECT_EQ(screen.at(31,2).style.foreground,custom);
    EXPECT_TRUE(screen.at(31,2).style.bold);
}
