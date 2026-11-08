/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <terion/terion.h>
#include <gtest/gtest.h>

using namespace terion;

TEST(Layout, Behavior)
{
    Reconciler flexible;
    flexible.update(Column({.children = {
        Typography({.text = "Header"}),
        Row({.children = {
            Box({.children = {Typography({.text = "L"})},.layout = {.width = 5}}),
            Box({.children = {Typography({.text = "R"})},.layout = {.flex = 1}})
        },.layout = {.flex = 1,.alignItems = AlignItems::Stretch}}),
        Typography({.text = "Footer"})
    },.layout = {.width = 30,.height = 10,.alignItems = AlignItems::Stretch}}));
    flexible.root()->layout(Constraints{.maximum = {.width = 30,.height = 10}});
    const auto& flex_children = flexible.root()->children();
    EXPECT_EQ(flex_children[0]->bounds().height,1);
    EXPECT_EQ(flex_children[1]->bounds().height,8);
    EXPECT_EQ(flex_children[2]->bounds().y,9);
    EXPECT_EQ(flex_children[1]->children()[0]->bounds().width,5);
    EXPECT_EQ(flex_children[1]->children()[1]->bounds().width,25);
    EXPECT_EQ(flex_children[1]->children()[0]->bounds().height,8);

    flexible.update(Row({.children = {
        Typography({.text = "A"}),Typography({.text = "B"})
    },.layout = {.width = 12,.justifyContent = JustifyContent::SpaceBetween,
        .alignItems = AlignItems::End}}));
    flexible.root()->layout(Constraints{.maximum = {.width = 12,.height = 4}});
    EXPECT_EQ(flexible.root()->children()[0]->bounds().x,0);
    EXPECT_EQ(flexible.root()->children()[1]->bounds().x,11);

    flexible.update(Box({.children = {Typography({.text = "X"})},
        .layout = {.width = 20,.minWidth = 3,.maxWidth = 8}}));
    EXPECT_EQ(flexible.root()->measure(Constraints{.maximum = {.width = 40,.height = 10}}).width,8);

}
