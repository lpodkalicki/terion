/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <gtest/gtest.h>
#include <terion/terion.h>

using namespace terion;

TEST(RadioGroup, ArrowsSkipDisabledOptionsAndWrap)
{
    std::string selected;
    Reconciler tree;
    tree.update(RadioGroup({.options = {{"Auto","auto"},{"Disabled","locked",true},{"Off","off"}},
        .value = "auto",.onChange = [&](std::string value) { selected = std::move(value); }}));
    EXPECT_TRUE(tree.root()->handleChoiceKey({.key = Key::Down}));
    EXPECT_EQ(selected,"off");
    tree.update(RadioGroup({.options = {{"Auto","auto"},{"Disabled","locked",true},{"Off","off"}},
        .value = "off",.onChange = [&](std::string value) { selected = std::move(value); }}));
    EXPECT_TRUE(tree.root()->handleChoiceKey({.key = Key::Down}));
    EXPECT_EQ(selected,"auto");
}

TEST(RadioGroup, ClickSelectsRowButNotDisabledOption)
{
    std::string selected;
    Reconciler tree;
    tree.update(RadioGroup({.options = {{"Auto","auto"},{"Disabled","locked",true},{"Off","off"}},
        .value = "auto",.onChange = [&](std::string value) { selected = std::move(value); }}));
    tree.root()->layout({.maximum = {.width = 30,.height = 10}});
    EXPECT_TRUE(tree.root()->handleChoiceClick({.x = 0,.y = 1}));
    EXPECT_TRUE(selected.empty());
    EXPECT_TRUE(tree.root()->handleChoiceClick({.x = 0,.y = 2}));
    EXPECT_EQ(selected,"off");
}
