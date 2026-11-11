/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <gtest/gtest.h>
#include <terion/terion.h>

using namespace terion;

TEST(Tooltip, MeasuresLikeWrappedChild)
{
    Reconciler wrapped;
    Reconciler plain;
    wrapped.update(Tooltip({.child = Typography({.text = "Action"}),.text = "Details"}));
    plain.update(Typography({.text = "Action"}));
    EXPECT_EQ(wrapped.root()->measure({.maximum = {.width = 40,.height = 10}}),
        plain.root()->measure({.maximum = {.width = 40,.height = 10}}));
}

TEST(Tooltip, PositionsPopupInsideTerminal)
{
    Reconciler tree;
    tree.update(Tooltip({.child = Box({.children = {Typography({.text = "Area"})},
        .layout = {.width = 20,.height = 8}}),.text = "Helpful information"}));
    const Size screen {.width = 20,.height = 8};
    tree.root()->layout({.minimum = screen,.maximum = screen});
    const Rect popup = tree.root()->tooltipRect({.x = 19,.y = 7},screen);
    EXPECT_GT(popup.width,0);
    EXPECT_GE(popup.x,0);
    EXPECT_GE(popup.y,0);
    EXPECT_LE(popup.x+popup.width,screen.width);
    EXPECT_LE(popup.y+popup.height,screen.height);
}

TEST(Tooltip, RendersOnlyWhileHoveredAndHonorsDisabled)
{
    Reconciler tree;
    tree.update(Tooltip({.child = Typography({.text = "Target"}),.text = "Tip"}));
    const Size screen {.width = 30,.height = 10};
    tree.root()->layout({.maximum = screen});
    OverlayManager overlays;
    overlays.collect(tree.root());
    const auto paint = [&] {
        ScreenBuffer buffer(screen);
        Painter painter(buffer);
        overlays.paint(painter,screen,Theme::dark());
        return buffer;
    };
    const auto before = paint();
    overlays.set_hover(Point{.x = 0,.y = 0});
    const auto after = paint();
    const Rect popup = tree.root()->tooltipRect({.x = 0,.y = 0},screen);
    EXPECT_NE(before.at(popup.x,popup.y).grapheme,after.at(popup.x,popup.y).grapheme);
    overlays.set_hover(std::nullopt);
    const auto hidden = paint();
    EXPECT_EQ(hidden.at(popup.x,popup.y).grapheme,before.at(popup.x,popup.y).grapheme);

    tree.update(Tooltip({.child = Typography({.text = "Target"}),.text = "Tip",.disabled = true}));
    tree.root()->layout({.maximum = screen});
    overlays.collect(tree.root());
    overlays.set_hover(Point{.x = 0,.y = 0});
    const auto disabled = paint();
    EXPECT_EQ(disabled.at(popup.x,popup.y).grapheme,before.at(popup.x,popup.y).grapheme);
}
