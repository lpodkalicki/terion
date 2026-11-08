/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <terion/terion.h>
#include <gtest/gtest.h>

using namespace terion;

TEST(Grid, Behavior)
{
    Reconciler grid;
    grid.update(Grid({.children = {
        Typography({.text = "A"}), Typography({.text = "B"}),
        Typography({.text = "C"}), Typography({.text = "D"}),
        Typography({.text = "E"})
    },.layout = {.width = 11,.gap = 1},.columns = 2}));
    grid.root()->layout(Constraints{.maximum = {.width = 11,.height = 10}});
    EXPECT_EQ(grid.root()->children()[0]->bounds().x,0);
    EXPECT_EQ(grid.root()->children()[1]->bounds().x,6);
    EXPECT_EQ(grid.root()->children()[2]->bounds().y,2);
    EXPECT_EQ(grid.root()->children()[4]->bounds().y,4);
    ScreenBuffer grid_buffer({.width = 11,.height = 6});
    Painter grid_painter(grid_buffer);
    grid.root()->paint(grid_painter);
    EXPECT_EQ(grid_buffer.at(6,0).grapheme,"B");
    EXPECT_EQ(grid_buffer.at(0,4).grapheme,"E");

    Reconciler stack;
    stack.update(Stack({.children = {Typography({.text = "A"}),Typography({.text = "B"})},
        .direction = StackDirection::Horizontal}));
    stack.root()->layout(Constraints{.maximum = {.width = 10,.height = 2}});
    EXPECT_EQ(stack.root()->children()[1]->bounds().x,1);

}
