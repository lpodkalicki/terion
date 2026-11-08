/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <gtest/gtest.h>
#include <terion/terion.h>

using namespace terion;

TEST(ContextMenu, OpensAtRightClickAndClampsToScreen)
{
    Reconciler tree;
    tree.update(ContextMenu({.child = Box({.children = {Typography({.text = "Target"})},
        .layout = {.width = 32,.height = 12}}),
        .options = {{"Open","open"},{"Remove","remove"}}}));
    const Size screen {.width = 32,.height = 12};
    tree.root()->layout({.minimum = screen,.maximum = screen});
    auto* target = tree.root()->contextTarget({.x = 31,.y = 11});
    ASSERT_EQ(target,tree.root());
    EXPECT_TRUE(target->openContextMenu({.x = 31,.y = 11},screen));
    const auto popup = target->selectPopupRect(screen);
    EXPECT_EQ(popup.x+popup.width,screen.width);
    EXPECT_EQ(popup.y+popup.height,screen.height);
    EXPECT_TRUE(target->selectContains({.x = popup.x,.y = popup.y},screen));
}

TEST(ContextMenu, SelectsEnabledOptionUsingKeyboard)
{
    std::string action;
    Reconciler tree;
    tree.update(ContextMenu({.child = Typography({.text = "Target"}),
        .options = {{"Disabled","disabled",true},{"Open","open"},{"Delete","delete"}},
        .onSelect = [&](std::string value) { action = std::move(value); }}));
    const Size screen {.width = 30,.height = 10};
    tree.root()->layout({.maximum = screen});
    ASSERT_TRUE(tree.root()->openContextMenu({.x = 1,.y = 0},screen));
    EXPECT_TRUE(tree.root()->handleSelectKey({.key = Key::Enter},screen));
    EXPECT_EQ(action,"open");
    EXPECT_FALSE(tree.root()->selectOpen());

    ASSERT_TRUE(tree.root()->openContextMenu({.x = 1,.y = 0},screen));
    EXPECT_TRUE(tree.root()->handleSelectKey({.key = Key::Escape},screen));
    EXPECT_FALSE(tree.root()->selectOpen());
}

TEST(ContextMenu, MouseClickAndOutsideDismissal)
{
    std::string action;
    Reconciler tree;
    tree.update(ContextMenu({.child = Typography({.text = "Target"}),
        .options = {{"Open","open"},{"Delete","delete"}},
        .onSelect = [&](std::string value) { action = std::move(value); }}));
    const Size screen {.width = 40,.height = 12};
    tree.root()->layout({.maximum = screen});
    OverlayManager overlays;
    overlays.collect(tree.root());
    ASSERT_TRUE(tree.root()->openContextMenu({.x = 3,.y = 0},screen));
    const auto popup = tree.root()->selectPopupRect(screen);
    ASSERT_EQ(overlays.handleMouse({.position = {.x = popup.x+1,.y = popup.y+1},
        .button = MouseButton::Left,.action = MouseAction::Press},screen),tree.root());
    EXPECT_EQ(action,"delete");
    EXPECT_FALSE(tree.root()->selectOpen());
    ASSERT_TRUE(tree.root()->openContextMenu({.x = 3,.y = 0},screen));
    overlays.dismissOutside({.position = {.x = 39,.y = 11},
        .button = MouseButton::Left,.action = MouseAction::Press},screen);
    EXPECT_FALSE(tree.root()->selectOpen());
}
