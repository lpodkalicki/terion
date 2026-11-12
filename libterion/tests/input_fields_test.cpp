/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <terion/terion.h>
#include <gtest/gtest.h>

using namespace terion;

TEST(NumberField, AllowsPartialNumbersAndRejectsNonNumericInput)
{
    std::string value;
    Reconciler reconciler;
    reconciler.update(NumberField({.value = "",.on_change = [&](std::string next) { value = std::move(next); }}));
    auto* node = reconciler.root();
    EXPECT_TRUE(node->handleTextInput(Event{.key = Key::Character,.character = U'-'}));
    EXPECT_EQ(value,"-");
    EXPECT_TRUE(node->handleTextInput(Event{.key = Key::Character,.character = U'2'}));
    EXPECT_EQ(value,"-2");
    EXPECT_TRUE(node->handleTextInput(Event{.key = Key::Character,.character = U'x'}));
    EXPECT_EQ(value,"-2");
    EXPECT_TRUE(node->handleTextInput(Event{.key = Key::Character,.character = U'.'}));
    EXPECT_EQ(value,"-2.");
    EXPECT_TRUE(node->handleTextInput(Event{.key = Key::Character,.character = U'.'}));
    EXPECT_EQ(value,"-2.");
}

TEST(NumberField, ArrowKeysHonorRangeAndStep)
{
    std::string value;
    Reconciler reconciler;
    reconciler.update(NumberField({.value = "5",.min_value = 2,.max_value = 6,.step = 2,
        .on_change = [&](std::string next) { value = std::move(next); }}));
    auto* node = reconciler.root();
    EXPECT_TRUE(node->handleTextInput(Event{.key = Key::Up}));
    EXPECT_EQ(value,"6");
    EXPECT_TRUE(node->handleTextInput(Event{.key = Key::Down}));
    EXPECT_EQ(value,"4");
}

TEST(PasswordField, MasksUtf8AndRevealShowsActualCharacters)
{
    Reconciler reconciler;
    reconciler.update(PasswordField({.value = "łóA",.layout = {.width = 16,.height = 3}}));
    auto* node = reconciler.root();
    node->layout(Constraints{.maximum = {.width = 16,.height = 3}});
    ScreenBuffer buffer({.width = 16,.height = 3});
    Painter painter(buffer);
    node->paint(painter,{}, {},std::nullopt,nullptr);
    EXPECT_EQ(buffer.at(2,1).grapheme,"•");
    EXPECT_EQ(buffer.at(3,1).grapheme,"•");
    EXPECT_EQ(buffer.at(4,1).grapheme,"•");
    reconciler.update(PasswordField({.value = "łóA",.reveal = true,
        .layout = {.width = 16,.height = 3}}));
    node = reconciler.root();
    node->layout(Constraints{.maximum = {.width = 16,.height = 3}});
    ScreenBuffer visible({.width = 16,.height = 3});
    Painter visible_painter(visible);
    node->paint(visible_painter,{}, {},std::nullopt,nullptr);
    EXPECT_EQ(visible.at(2,1).grapheme,"ł");
}
