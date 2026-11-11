/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <chrono>
#include <gtest/gtest.h>
#include <terion/terion.h>

using namespace terion;

TEST(Toast, DoesNotTakeLayoutSpace)
{
    Reconciler tree;
    tree.update(Column({.children = {
        Typography({.text = "Normal content"}),
        Toast({.open = true,.message = "Saved"})
    },.layout = {.gap = 2}}));
    EXPECT_EQ(tree.root()->measure({.maximum = {.width = 50,.height = 20}}).height,1);
}

TEST(Toast, ExpiresAndCallsOnClose)
{
    bool closed = false;
    Reconciler tree;
    tree.update(Toast({.open = true,.message = "Imported",.duration_ms = 100,
        .on_close = [&] { closed = true; }}));
    OverlayManager overlays;
    overlays.collect(tree.root());
    EXPECT_TRUE(tree.root()->toast_open());
    EXPECT_FALSE(overlays.tick(std::chrono::steady_clock::now()));
    EXPECT_TRUE(overlays.tick(std::chrono::steady_clock::now()+std::chrono::seconds(1)));
    EXPECT_TRUE(closed);
    EXPECT_FALSE(tree.root()->toast_open());
    // Reconciliation with an unchanged open prop must not resurrect an expired toast.
    tree.update(Toast({.open = true,.message = "Imported",.duration_ms = 100}));
    EXPECT_FALSE(tree.root()->toast_open());
}

TEST(Toast, FitsSmallScreenAndDismissesOnClick)
{
    bool closed = false;
    Reconciler tree;
    tree.update(Toast({.open = true,.message = "A very long notification message",
        .severity = ToastSeverity::Warning,.duration_ms = 0,
        .on_close = [&] { closed = true; }}));
    const Size screen {.width = 16,.height = 5};
    const Rect popup = tree.root()->toastRect(screen);
    EXPECT_LE(popup.x+popup.width,screen.width);
    EXPECT_LE(popup.y+popup.height,screen.height);
    ScreenBuffer buffer(screen);
    Painter painter(buffer);
    OverlayManager overlays;
    overlays.collect(tree.root());
    overlays.paint(painter,screen,Theme::dark());
    EXPECT_TRUE(tree.root()->toast_open());
    EXPECT_EQ(overlays.handleMouse({.position = {.x = popup.x+1,.y = popup.y+1},
        .button = MouseButton::Left,.action = MouseAction::Press},screen),tree.root());
    EXPECT_TRUE(closed);
    EXPECT_FALSE(tree.root()->toast_open());
}

TEST(Toast, ReopensOnNewMessage)
{
    Reconciler tree;
    tree.update(Toast({.open = true,.message = "First",.duration_ms = 50}));
    tree.root()->closeToast();
    tree.update(Toast({.open = true,.message = "Second",.duration_ms = 50}));
    EXPECT_TRUE(tree.root()->toast_open());
}
