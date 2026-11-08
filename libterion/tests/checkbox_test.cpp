/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <gtest/gtest.h>
#include <terion/terion.h>

using namespace terion;

TEST(Checkbox, TogglesWithSpaceAndEnter)
{
    bool value = false;
    Reconciler tree;
    tree.update(Checkbox({.label = "Logging",.checked = false,.onChange = [&](bool next) { value = next; }}));
    auto* node = tree.root();
    EXPECT_TRUE(node->handleChoiceKey({.key = Key::Character,.character = U' '}));
    EXPECT_TRUE(value);
    tree.update(Checkbox({.label = "Logging",.checked = true,.onChange = [&](bool next) { value = next; }}));
    EXPECT_TRUE(node->handleChoiceKey({.key = Key::Enter}));
    EXPECT_FALSE(value);
}

TEST(Checkbox, DisabledDoesNotRespondToInput)
{
    int calls = 0;
    Reconciler tree;
    tree.update(Checkbox({.label = "Disabled",.disabled = true,
        .onChange = [&](bool) { ++calls; }}));
    EXPECT_FALSE(tree.root()->handleChoiceKey({.key = Key::Enter}));
    EXPECT_FALSE(tree.root()->handleChoiceClick({}));
    EXPECT_EQ(calls,0);
}

TEST(Checkbox, VariantsHaveDistinctMarkersAndAsciiFallback)
{
    Reconciler tree;
    ScreenBuffer buffer({.width = 20,.height = 2});
    Painter painter(buffer);
    const auto marker = [&](CheckboxVariant variant, bool unicode) {
        tree.update(Checkbox({.label = "Test",.checked = true,.variant = variant}));
        tree.root()->layout({.maximum = {.width = 20,.height = 2}});
        buffer.clear();
        tree.root()->paint(painter,{},{},std::nullopt,nullptr,unicode);
        return buffer.at(0,0).grapheme;
    };
    EXPECT_EQ(marker(CheckboxVariant::Default,true),"■");
    EXPECT_EQ(marker(CheckboxVariant::Unicode,true),"▣");
    EXPECT_NE(marker(CheckboxVariant::Default,true),marker(CheckboxVariant::Unicode,true));
    EXPECT_NE(marker(CheckboxVariant::Unicode,true),marker(CheckboxVariant::Emoji,true));
    EXPECT_EQ(marker(CheckboxVariant::Emoji,false),"[");
}
