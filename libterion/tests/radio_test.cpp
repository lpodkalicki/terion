/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <gtest/gtest.h>
#include <terion/terion.h>

using namespace terion;

TEST(Radio, SelectsOnlyWhenUnchecked)
{
    int calls = 0;
    Reconciler tree;
    tree.update(Radio({.label = "Red",.onChange = [&](bool checked) {
        EXPECT_TRUE(checked);
        ++calls;
    }}));
    EXPECT_TRUE(tree.root()->handleChoiceKey({.key = Key::Enter}));
    EXPECT_EQ(calls,1);
    tree.update(Radio({.label = "Red",.checked = true,.onChange = [&](bool) { ++calls; }}));
    EXPECT_TRUE(tree.root()->handleChoiceKey({.key = Key::Enter}));
    EXPECT_TRUE(tree.root()->handleChoiceClick({}));
    EXPECT_EQ(calls,1);
}
