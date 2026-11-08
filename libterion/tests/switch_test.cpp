/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <gtest/gtest.h>
#include <terion/terion.h>

using namespace terion;

TEST(Switch, ClickTogglesCurrentValue)
{
    bool next = false;
    Reconciler tree;
    tree.update(Switch({.label = "Dark mode",.checked = true,
        .onChange = [&](bool checked) { next = checked; }}));
    EXPECT_TRUE(tree.root()->handleChoiceClick({}));
    EXPECT_FALSE(next);
    tree.update(Switch({.label = "Dark mode",.checked = false,
        .onChange = [&](bool checked) { next = checked; }}));
    EXPECT_TRUE(tree.root()->handleChoiceClick({}));
    EXPECT_TRUE(next);
}

TEST(Switch, DrawsDistinctOnAndOffStates)
{
    Reconciler tree;
    tree.update(Switch({.label = "Telemetry",.checked = true}));
    tree.root()->layout({.maximum = {.width = 40,.height = 3}});
    ScreenBuffer buffer({.width = 40,.height = 3});
    Painter painter(buffer);
    tree.root()->paint(painter);
    const auto on = buffer.at(2,0).grapheme;
    EXPECT_EQ(on,"●");
    tree.update(Switch({.label = "Telemetry",.checked = false}));
    tree.root()->layout({.maximum = {.width = 40,.height = 3}});
    tree.root()->paint(painter);
    EXPECT_EQ(buffer.at(0,0).grapheme,"○");
    EXPECT_NE(buffer.at(0,0).grapheme,on);
}

TEST(Switch, EmojiVariantFallsBackToAsciiWhenUnicodeIsUnavailable)
{
    Reconciler tree;
    tree.update(Switch({.label = "Test",.checked = true,.variant = CheckboxVariant::Emoji}));
    tree.root()->layout({.maximum = {.width = 20,.height = 2}});
    ScreenBuffer buffer({.width = 20,.height = 2});
    Painter painter(buffer);
    tree.root()->paint(painter,{},{},std::nullopt,nullptr,false);
    EXPECT_EQ(buffer.at(0,0).grapheme,"[");
}
