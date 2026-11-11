/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <gtest/gtest.h>
#include <terion/terion.h>

using namespace terion;

TEST(Tabs, ArrowKeysSkipDisabledTabsAndWrap)
{
    std::string selected;
    Reconciler tree;
    const auto build = [&](std::string value) {
        tree.update(Tabs({.options = {{"One","one"},{"Disabled","disabled",true},{"Three","three"}},
            .value = std::move(value),.on_change = [&](std::string next) { selected = std::move(next); }}));
    };
    build("one");
    EXPECT_TRUE(tree.root()->handleTabsKey({.key = Key::Right}));
    EXPECT_EQ(selected,"three");
    build("three");
    EXPECT_TRUE(tree.root()->handleTabsKey({.key = Key::Right}));
    EXPECT_EQ(selected,"one");
    build("three");
    EXPECT_TRUE(tree.root()->handleTabsKey({.key = Key::Home}));
    EXPECT_EQ(selected,"one");
}

TEST(Tabs, MouseClickChangesSelectedTab)
{
    std::string selected;
    Reconciler tree;
    tree.update(Tabs({.options = {{"One","one"},{"Two","two"},{"Three","three"}},
        .value = "one",.on_change = [&](std::string next) { selected = std::move(next); }}));
    tree.root()->layout({.maximum = {.width = 50,.height = 5}});
    EXPECT_TRUE(tree.root()->handleTabsClick({.x = 8,.y = 1}));
    EXPECT_EQ(selected,"two");
    EXPECT_FALSE(tree.root()->handleTabsClick({.x = 40,.y = 1}));
}

TEST(Tabs, MouseHoverHighlightsOnlyTheTabUnderPointer)
{
    Reconciler tree;
    tree.update(Tabs({.options = {{"One","one"},{"Two","two"},{"Three","three"}},
        .value = "three"}));
    auto* tabs = tree.root();
    tabs->layout({.maximum = {.width = 50,.height = 5}});
    const Theme theme = Theme::dark();
    ScreenBuffer buffer({.width = 50,.height = 5});
    Painter painter(buffer);
    EXPECT_TRUE(tabs->updateTabsHover({.x = 8,.y = 1}));
    tabs->paint(painter,{},std::to_string(tabs->id()),std::nullopt,&theme);
    EXPECT_EQ(buffer.at(1,0).style.background,theme.palette.background);
    EXPECT_NE(buffer.at(8,0).style.background,theme.palette.background);
    EXPECT_TRUE(tabs->updateTabsHover({.x = 1,.y = 1}));
    buffer.clear();
    tabs->paint(painter,{},std::to_string(tabs->id()),std::nullopt,&theme);
    EXPECT_NE(buffer.at(1,0).style.background,theme.palette.background);
    EXPECT_EQ(buffer.at(8,0).style.background,theme.palette.background);
    EXPECT_FALSE(tabs->updateTabsHover({.x = 1,.y = 1}));
}

TEST(TabPanel, InactivePanelDoesNotOccupyLayoutSpace)
{
    Reconciler tree;
    tree.update(Column({.children = {
        TabPanel({.children = {Typography({.text = "Hidden"})},.key = "hidden",.active = false}),
        TabPanel({.children = {Typography({.text = "Visible"})},.key = "visible",.active = true}),
        Typography({.text = "Footer"})
    },.layout = {.gap = 2}}));
    tree.root()->layout({.maximum = {.width = 50,.height = 20}});
    const auto& nodes = tree.root()->children();
    EXPECT_EQ(nodes[0]->bounds().height,0);
    EXPECT_EQ(nodes[1]->bounds().y,0);
    EXPECT_EQ(nodes[2]->bounds().y,nodes[1]->bounds().height+2);
    EXPECT_EQ(nodes[0]->hitTest({.x = 0,.y = 0}),nullptr);
}

TEST(TabPanel, SwitchingPanelsUpdatesFocusTargets)
{
    State<std::string> active{"one"};
    Application app(std::make_unique<HeadlessTerminalBackend>(Size{.width = 50,.height = 20}));
    app.render([&] {
        const auto page = active.get();
        return Column({.children = {
            Tabs({.options = {{"One","one"},{"Two","two"}},.value = page,.key = "tabs",
                .on_change = [&](std::string value) { active.set(std::move(value)); }}),
            TabPanel({.children = {Button({.text = "First",.key = "first",
                .on_click = [] {}})},.key = "panel-one",.active = page == "one"}),
            TabPanel({.children = {Button({.text = "Second",.key = "second",
                .on_click = [] {}})},.key = "panel-two",.active = page == "two"})
        }});
    });
    app.renderOnce();
    const auto* root = app.reconciler().root();
    const auto first_id = std::to_string(root->children()[1]->children()[0]->id());
    const auto second_id = std::to_string(root->children()[2]->children()[0]->id());
    EXPECT_TRUE(app.focus().focus(first_id));
    active.set("two");
    app.renderOnce();
    EXPECT_FALSE(app.focus().focus(first_id));
    EXPECT_TRUE(app.focus().focus(second_id));
}
