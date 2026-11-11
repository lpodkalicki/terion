/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <terion/terion.h>
#include <gtest/gtest.h>

using namespace terion;

TEST(OverlayManager, TopModalBlocksLowerDialogAndBackgroundPopup)
{
    int lower_closes = 0;
    int upper_closes = 0;
    const Size screen{.width = 60,.height = 20};
    Reconciler tree;
    tree.update(Column({.children = {
        ContextMenu({.child = Box({.layout = {.width = 30,.height = 10}}),
            .options = {{"Open","open"}}}),
        Dialog({.open = true,.title = "Lower",.on_close = [&] { ++lower_closes; }}),
        Dialog({.open = true,.title = "Upper",.on_close = [&] { ++upper_closes; }}),
        Toast({.open = true,.message = "Background notification",.duration_ms = 0})
    }}));
    tree.root()->layout({.maximum = screen});
    auto* background = tree.root()->children()[0].get();
    auto* lower = tree.root()->children()[1].get();
    auto* upper = tree.root()->children()[2].get();
    ASSERT_TRUE(background->openContextMenu({.x = 2,.y = 2},screen));

    OverlayManager overlays;
    overlays.collect(tree.root());
    EXPECT_EQ(overlays.activeDialog(),upper);
    EXPECT_TRUE(overlays.modal_open());
    EXPECT_FALSE(background->selectOpen());
    EXPECT_EQ(overlays.handleMouse({.position = {.x = 1,.y = 1},
        .button = MouseButton::Left,.action = MouseAction::Move},screen),upper);
    EXPECT_EQ(lower_closes,0);
    EXPECT_EQ(upper_closes,0);
    EXPECT_TRUE(lower->dialog_open());

    EXPECT_TRUE(overlays.handleKey({.key = Key::Escape}));
    EXPECT_EQ(lower_closes,0);
    EXPECT_EQ(upper_closes,1);
}

TEST(OverlayManager, ModalPreventsDismissalOfItsOwnPopup)
{
    const Size screen{.width = 60,.height = 20};
    Reconciler tree;
    tree.update(Dialog({.open = true,.title = "Modal",.children = {
        ContextMenu({.child = Box({.layout = {.width = 20,.height = 4}}),
            .options = {{"Save","save"}}})
    }}));
    tree.root()->layoutDialogChildren(screen);
    auto* inside = tree.root()->children().front().get();
    ASSERT_TRUE(inside->openContextMenu({.x = 5,.y = 5},screen));

    OverlayManager overlays;
    overlays.collect(tree.root());
    EXPECT_EQ(overlays.activeDialog(),tree.root());
    EXPECT_TRUE(inside->selectOpen());
    overlays.dismissOutside({.position = {.x = 59,.y = 19},
        .button = MouseButton::Left,.action = MouseAction::Press},screen);
    EXPECT_TRUE(inside->selectOpen());
    EXPECT_EQ(overlays.handleMouse({.position = {.x = 59,.y = 19},
        .button = MouseButton::Left,.action = MouseAction::Press},screen),tree.root());
    EXPECT_FALSE(inside->selectOpen());
}

TEST(OverlayManager, EscapeDismissesNestedPopupBeforeParentDialog)
{
    int closes = 0;
    const Size screen{.width = 60,.height = 20};
    Reconciler tree;
    tree.update(Dialog({.open = true,.title = "Modal",.on_close = [&] { ++closes; },.children = {
        ContextMenu({.child = Box({.layout = {.width = 20,.height = 4}}),
            .options = {{"Save","save"}}})
    }}));
    tree.root()->layoutDialogChildren(screen);
    auto* popup = tree.root()->children().front().get();
    ASSERT_TRUE(popup->openContextMenu({.x = 5,.y = 5},screen));
    OverlayManager overlays;
    overlays.collect(tree.root());
    ASSERT_EQ(overlays.activePopup(),popup);
    EXPECT_TRUE(overlays.handleKey({.key = Key::Escape}));
    EXPECT_FALSE(popup->selectOpen());
    EXPECT_EQ(closes,0);
    EXPECT_TRUE(overlays.handleKey({.key = Key::Escape}));
    EXPECT_EQ(closes,1);
}

TEST(OverlayManager, HighestPopupZIndexReceivesMouseFirst)
{
    const Size screen{.width = 40,.height = 12};
    std::string selected;
    Reconciler lower;
    lower.update(ContextMenu({.child = Box({.layout = {.width = 20,.height = 5}}),
        .options = {{"Lower","lower"}},.on_select = [&](std::string) { selected = "lower"; }}));
    Reconciler upper;
    upper.update(ContextMenu({.child = Box({.layout = {.width = 20,.height = 5}}),
        .options = {{"Upper","upper"}},.on_select = [&](std::string) { selected = "upper"; }}));
    lower.root()->layout({.maximum = screen});
    upper.root()->layout({.maximum = screen});
    ASSERT_TRUE(lower.root()->openContextMenu({.x = 2,.y = 1},screen));
    ASSERT_TRUE(upper.root()->openContextMenu({.x = 2,.y = 1},screen));
    OverlayManager overlays;
    overlays.addPortal(*upper.root(),20);
    overlays.addPortal(*lower.root(),10);
    const Rect popup = upper.root()->selectPopupRect(screen);
    EXPECT_EQ(overlays.handleMouse({.position = {.x = popup.x+1,.y = popup.y},
        .button = MouseButton::Left,.action = MouseAction::Press},screen),upper.root());
    EXPECT_EQ(selected,"upper");
}
