/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <gtest/gtest.h>
#include <terion/terion.h>

using namespace terion;

TEST(DropdownMenu, OpensAndInvokesActionWithKeyboard)
{
    std::string action;
    Reconciler tree;
    tree.update(DropdownMenu({.label = "Actions",.options = {
        {"New", "new"}, {"Disabled", "disabled",true}, {"Open", "open"}},
        .onSelect = [&](std::string value) { action = std::move(value); }}));
    const Size screen {.width = 40,.height = 15};
    tree.root()->layout({.maximum = screen});
    EXPECT_TRUE(tree.root()->handleSelectKey({.key = Key::Enter},screen));
    EXPECT_TRUE(tree.root()->selectOpen());
    EXPECT_TRUE(tree.root()->handleSelectKey({.key = Key::Down},screen));
    EXPECT_TRUE(tree.root()->handleSelectKey({.key = Key::Enter},screen));
    EXPECT_EQ(action,"open");
    EXPECT_FALSE(tree.root()->selectOpen());
}

TEST(DropdownMenu, MouseSelectionAndEscape)
{
    std::string action;
    Reconciler tree;
    tree.update(DropdownMenu({.label = "File",.options = {{"New","new"},{"Open","open"}},
        .onSelect = [&](std::string value) { action = std::move(value); }}));
    const Size screen {.width = 40,.height = 15};
    tree.root()->layout({.maximum = screen});
    EXPECT_TRUE(tree.root()->handleSelectMouse({.position = {.x = 1,.y = 1},
        .button = MouseButton::Left,.action = MouseAction::Press},screen));
    const Rect popup = tree.root()->selectPopupRect(screen);
    EXPECT_GT(popup.width,0);
    EXPECT_TRUE(tree.root()->handleSelectMouse({.position = {.x = popup.x+1,.y = popup.y+1},
        .button = MouseButton::Left,.action = MouseAction::Press},screen));
    EXPECT_EQ(action,"open");
    EXPECT_FALSE(tree.root()->selectOpen());
    EXPECT_TRUE(tree.root()->handleSelectKey({.key = Key::Enter},screen));
    EXPECT_TRUE(tree.root()->handleSelectKey({.key = Key::Escape},screen));
    EXPECT_FALSE(tree.root()->selectOpen());
}

TEST(DropdownMenu, OverlayPaintsAboveUnderlyingText)
{
    Reconciler tree;
    tree.update(DropdownMenu({.label = "Menu",.options = {{"Long option","long"}}}));
    const Size screen {.width = 30,.height = 8};
    tree.root()->layout({.maximum = screen});
    EXPECT_TRUE(tree.root()->handleSelectKey({.key = Key::Enter},screen));
    const Rect popup = tree.root()->selectPopupRect(screen);
    ScreenBuffer buffer(screen);
    Painter painter(buffer);
    painter.text(popup.x+3,popup.y,"Underlay text");
    tree.root()->paintOverlay(painter,screen,Theme::dark());
    EXPECT_EQ(buffer.at(popup.x+1,popup.y).grapheme,"L");
    EXPECT_EQ(buffer.at(popup.x+popup.width-1,popup.y).grapheme," ");
}

TEST(DropdownMenu, ButtonVariantsRenderWithoutSelectFieldChrome)
{
    Reconciler tree;
    const auto draw = [&](ButtonVariant variant) {
        tree.update(DropdownMenu({.label = "File",.options = {{"New","new"}},.variant = variant}));
        tree.root()->layout({.maximum = {.width = 30,.height = 5}});
        ScreenBuffer buffer({.width = 30,.height = 5});
        Painter painter(buffer);
        tree.root()->paint(painter);
        return buffer.at(0,0).grapheme;
    };
    EXPECT_NE(draw(ButtonVariant::Text),"╭");
    EXPECT_EQ(draw(ButtonVariant::Outlined),"╭");
    EXPECT_NE(draw(ButtonVariant::Contained),"╭");
}
