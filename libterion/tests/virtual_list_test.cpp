/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <gtest/gtest.h>
#include <terion/terion.h>

using namespace terion;

TEST(VirtualList, PaintsOnlyVisibleRows)
{
    std::vector<int> rendered;
    Reconciler tree;
    tree.update(VirtualList({.item_count = 100000,
        .item_text = [&](int index) { rendered.push_back(index); return "Entry "+std::to_string(index); },
        .layout = {.width = 22,.height = 4}}));
    ASSERT_NE(tree.root(),nullptr);
    ASSERT_TRUE(tree.root()->children().empty());
    tree.root()->layout({.maximum = {.width = 30,.height = 10}});
    ScreenBuffer screen({.width = 30,.height = 10});
    Painter painter(screen);
    tree.root()->paint(painter);
    EXPECT_EQ(rendered,(std::vector<int>{0,1,2,3}));
    EXPECT_EQ(screen.at(0,0).grapheme,"E");
    rendered.clear();
    EXPECT_TRUE(tree.root()->scrollBy(1000));
    tree.root()->paint(painter);
    EXPECT_EQ(rendered,(std::vector<int>{1000,1001,1002,1003}));
    EXPECT_EQ(tree.root()->virtual_list_offset(),1000);
}

TEST(VirtualList, KeyboardRevealsSelectionAndMouseSelectsVisibleRow)
{
    int selected = 0;
    Reconciler tree;
    const auto component = [&] {
        return VirtualList({.item_count = 100,.selected_index = selected,
            .on_select = [&](int index) { selected = index; },
            .layout = {.width = 20,.height = 4}});
    };
    tree.update(component());
    tree.root()->layout({.maximum = {.width = 25,.height = 10}});
    EXPECT_TRUE(tree.root()->handleVirtualListKey({.key = Key::PageDown}));
    EXPECT_EQ(selected,4);
    EXPECT_EQ(tree.root()->virtual_list_offset(),1);
    tree.update(component());
    tree.root()->layout({.maximum = {.width = 25,.height = 10}});
    EXPECT_TRUE(tree.root()->handleVirtualListClick({.x = 1,.y = 2}));
    EXPECT_EQ(selected,3);
    EXPECT_TRUE(tree.root()->handleVirtualListKey({.key = Key::End}));
    EXPECT_EQ(selected,99);
    EXPECT_EQ(tree.root()->virtual_list_offset(),96);
}

TEST(VirtualList, ScrollbarSharesOffsetWithWheelAndClampsOnDataShrink)
{
    Reconciler tree;
    tree.update(VirtualList({.item_count = 1000,.layout = {.width = 20,.height = 6}}));
    tree.root()->layout({.maximum = {.width = 20,.height = 6}});
    const Rect track = tree.root()->scrollbarTrack();
    ASSERT_EQ(track.height,6);
    ASSERT_EQ(track.x,19);
    EXPECT_TRUE(tree.root()->scrollBy(600));
    EXPECT_EQ(tree.root()->virtual_list_offset(),600);
    int grab = 0;
    EXPECT_TRUE(tree.root()->scrollbarPress({.x = track.x,.y = track.y+track.height-1},grab));
    EXPECT_EQ(tree.root()->virtual_list_offset(),994);
    tree.update(VirtualList({.item_count = 3,.layout = {.width = 20,.height = 6}}));
    tree.root()->layout({.maximum = {.width = 20,.height = 6}});
    EXPECT_EQ(tree.root()->virtual_list_offset(),0);
    EXPECT_EQ(tree.root()->scrollbarTrack().height,0);
}

TEST(VirtualList, EmptyAndDisabledListsDoNotSelect)
{
    int selected = -1;
    Reconciler tree;
    tree.update(VirtualList({.item_count = 0,
        .on_select = [&](int index) { selected = index; },
        .layout = {.width = 14,.height = 4}}));
    tree.root()->layout({.maximum = {.width = 14,.height = 4}});
    EXPECT_FALSE(tree.root()->handleVirtualListKey({.key = Key::Down}));
    EXPECT_FALSE(tree.root()->handleVirtualListClick({.x = 1,.y = 0}));
    tree.update(VirtualList({.item_count = 20,
        .on_select = [&](int index) { selected = index; },
        .layout = {.width = 14,.height = 4},
        .disabled = true}));
    tree.root()->layout({.maximum = {.width = 14,.height = 4}});
    EXPECT_FALSE(tree.root()->handleVirtualListKey({.key = Key::Down}));
    EXPECT_FALSE(tree.root()->handleVirtualListClick({.x = 1,.y = 0}));
    EXPECT_EQ(selected,-1);
}
