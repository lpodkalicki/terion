/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <gtest/gtest.h>
#include <terion/terion.h>

using namespace terion;

TEST(TreeView, FlattensOnlyExpandedBranches)
{
    Reconciler tree;
    tree.update(TreeView({.items = {{"parent","Parent",{{"child","Child",{{"leaf","Leaf"}}}}}}}));
    ASSERT_EQ(tree.root()->tree_visible_items().size(),1U);
    tree.update(TreeView({.items = {{"parent","Parent",{{"child","Child",{{"leaf","Leaf"}}}}}},
        .expanded = {"parent"}}));
    const auto items = tree.root()->tree_visible_items();
    ASSERT_EQ(items.size(),2U);
    EXPECT_EQ(items[1].item->key,"child");
    EXPECT_EQ(items[1].parent,"parent");
    EXPECT_EQ(items[1].depth,1);
}

TEST(TreeView, KeyboardNavigatesAndTogglesExpandedNodes)
{
    std::string selected;
    std::string toggled;
    bool opened = false;
    Reconciler tree;
    tree.update(TreeView({.items = {{"root","Root",{{"first","First"},{"second","Second"}}}},
        .value = "root",.on_change = [&](std::string value) { selected = std::move(value); },
        .on_toggle = [&](std::string key,bool value) { toggled = std::move(key); opened = value; }}));
    tree.root()->layout({.maximum = {.width = 30,.height = 8}});
    EXPECT_TRUE(tree.root()->handleTreeKey({.key = Key::Right}));
    EXPECT_EQ(toggled,"root");
    EXPECT_TRUE(opened);
    tree.update(TreeView({.items = {{"root","Root",{{"first","First"},{"second","Second"}}}},
        .expanded = {"root"},.value = "root",
        .on_change = [&](std::string value) { selected = std::move(value); },
        .on_toggle = [&](std::string key,bool value) { toggled = std::move(key); opened = value; }}));
    tree.root()->layout({.maximum = {.width = 30,.height = 8}});
    EXPECT_TRUE(tree.root()->handleTreeKey({.key = Key::Down}));
    EXPECT_EQ(selected,"first");
    EXPECT_TRUE(tree.root()->handleTreeKey({.key = Key::Left}));
    EXPECT_EQ(toggled,"root");
    EXPECT_FALSE(opened);
}

TEST(TreeView, MouseSelectsLabelAndTogglesDisclosure)
{
    std::string selected;
    bool expanded = false;
    Reconciler tree;
    tree.update(TreeView({.items = {{"root","Root",{{"child","Child"}}}},
        .value = "root",.on_change = [&](std::string value) { selected = std::move(value); },
        .on_toggle = [&](std::string,bool value) { expanded = value; }}));
    tree.root()->layout({.maximum = {.width = 30,.height = 8}});
    EXPECT_TRUE(tree.root()->handleTreeClick({.x = 0,.y = 0}));
    EXPECT_TRUE(expanded);
    tree.update(TreeView({.items = {{"root","Root",{{"child","Child"}}}},
        .expanded = {"root"},.value = "root",
        .on_change = [&](std::string value) { selected = std::move(value); }}));
    tree.root()->layout({.maximum = {.width = 30,.height = 8}});
    EXPECT_TRUE(tree.root()->handleTreeClick({.x = 5,.y = 1}));
    EXPECT_EQ(selected,"child");
}

TEST(TreeView, ScrollbarClipsAndScrollsLongTrees)
{
    Reconciler tree;
    std::vector<TreeItem> items;
    for (int i = 0; i < 25; ++i)
        items.push_back({.key = std::to_string(i),.label = "Node " + std::to_string(i)});
    tree.update(TreeView({.items = std::move(items),.layout = {.width = 20,.height = 5}}));
    tree.root()->layout({.maximum = {.width = 20,.height = 5}});
    auto* view = tree.root();
    const auto track = view->scrollbarTrack();
    ASSERT_EQ(track.height,5);
    EXPECT_TRUE(view->scrollBy(10));
    EXPECT_GT(view->scrollbarThumb().y,track.y);
    ScreenBuffer screen({.width = 20,.height = 5});
    Painter painter(screen);
    view->paint(painter);
    EXPECT_EQ(screen.at(2,0).grapheme,"N");
    int grab = 0;
    EXPECT_TRUE(view->scrollbarPress({.x = track.x,.y = track.y+4},grab));
    EXPECT_EQ(view->scrollbarThumb().y+view->scrollbarThumb().height,track.y+track.height);
}

TEST(TreeView, KeyboardSkipsDisabledItemsAtBothEdges)
{
    std::string selected;
    Reconciler tree;
    tree.update(TreeView({.items = {
        {"first","First",{},true},{"middle","Middle"},{"last","Last",{},true}},
        .value = "middle",.on_change = [&](std::string value) { selected = std::move(value); }}));
    tree.root()->layout({.maximum = {.width = 30,.height = 5}});
    EXPECT_TRUE(tree.root()->handleTreeKey({.key = Key::Home}));
    EXPECT_TRUE(selected.empty());
    EXPECT_TRUE(tree.root()->handleTreeKey({.key = Key::End}));
    EXPECT_TRUE(selected.empty());
    tree.update(TreeView({.items = {
        {"first","First",{},true},{"middle","Middle"},{"last","Last",{},true}},
        .on_change = [&](std::string value) { selected = std::move(value); }}));
    tree.root()->layout({.maximum = {.width = 30,.height = 5}});
    EXPECT_TRUE(tree.root()->handleTreeKey({.key = Key::Home}));
    EXPECT_EQ(selected,"middle");
}

TEST(TreeView, RevealsExternallySelectedItemWithoutResettingManualScroll)
{
    Reconciler tree;
    std::vector<TreeItem> items;
    for (int i = 0; i < 20; ++i)
        items.push_back({.key = std::to_string(i),.label = "Node " + std::to_string(i)});
    const auto build = [&](std::string value) {
        tree.update(TreeView({.items = items,.value = std::move(value),
            .layout = {.width = 20,.height = 5}}));
        tree.root()->layout({.maximum = {.width = 20,.height = 5}});
    };
    build("0");
    build("19");
    EXPECT_EQ(tree.root()->scrollbarThumb().y+tree.root()->scrollbarThumb().height,5);
    EXPECT_TRUE(tree.root()->scrollBy(-5));
    const auto manual_position = tree.root()->scrollbarThumb().y;
    build("19");
    EXPECT_EQ(tree.root()->scrollbarThumb().y,manual_position);
}

TEST(TreeView, RejectsDuplicateOrEmptyItemKeys)
{
    Reconciler tree;
    const TreeView duplicate({.items = {{"x","One"},{"x","Two"}}});
    const TreeView empty({.items = {{"","Unnamed"}}});
    const TreeView nested({.items = {{"x","Root",{{"x","Child"}}}}});
    EXPECT_THROW(tree.update(duplicate),std::invalid_argument);
    EXPECT_THROW(tree.update(empty),std::invalid_argument);
    EXPECT_THROW(tree.update(nested),std::invalid_argument);
}

TEST(TreeView, PaintsHierarchyGuides)
{
    Reconciler tree;
    tree.update(TreeView({.items = {
        {"root-a","A",{{"child","Child"}}},{"root-b","B"}},
        .expanded = {"root-a"},.layout = {.width = 24,.height = 3}}));
    tree.root()->layout({.maximum = {.width = 24,.height = 3}});
    ScreenBuffer screen({.width = 24,.height = 3});
    Painter painter(screen);
    tree.root()->paint(painter);
    EXPECT_EQ(screen.at(1,1).grapheme,"└");
    EXPECT_EQ(screen.at(2,1).grapheme,"·");
}

TEST(TreeView, CollapsingBranchMovesSelectionFromHiddenDescendantToParent)
{
    std::string selection;
    bool opened = true;
    Reconciler tree;
    tree.update(TreeView({.items = {{"root","Root",{{"child","Child"}}}},
        .expanded = {"root"},.value = "child",
        .on_change = [&](std::string value) { selection = std::move(value); },
        .on_toggle = [&](std::string,bool value) { opened = value; }}));
    tree.root()->layout({.maximum = {.width = 25,.height = 4}});
    EXPECT_TRUE(tree.root()->handleTreeClick({.x = 0,.y = 0}));
    EXPECT_FALSE(opened);
    EXPECT_EQ(selection,"root");
}
