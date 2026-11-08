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
