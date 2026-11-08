/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <terion/terion.h>
#include <gtest/gtest.h>

using namespace terion;

TEST(Reconciler, Behavior)
{
    Reconciler reconciler;
    reconciler.update(Component::column({
        Component::text("Alpha","a"),
        Component::text("Beta","b")
    }));
    const auto* root = reconciler.root();
    ASSERT_TRUE(root != nullptr);
    ASSERT_EQ(root->children().size(),2u);
    const auto root_id = root->id();
    const auto a_id = root->children()[0]->id();
    const auto b_id = root->children()[1]->id();
    reconciler.update(Component::column({
        Component::text("Beta updated","b"),
        Component::text("Alpha","a")
    }));
    EXPECT_EQ(reconciler.root()->id(),root_id);
    EXPECT_EQ(reconciler.root()->children()[0]->id(),b_id);
    EXPECT_EQ(reconciler.root()->children()[0]->text(),"Beta updated");
    EXPECT_EQ(reconciler.root()->children()[1]->id(),a_id);
    reconciler.update(Component::column({Component::column({},"b")}));
    EXPECT_NE(reconciler.root()->children()[0]->id(),b_id);
    bool duplicate_rejected = false;
    try
    {
        reconciler.update(Component::column({Component::text("A","same"),Component::text("B","same")}));
    }
    catch (const std::invalid_argument&)
    {
        duplicate_rejected = true;
    }
    EXPECT_TRUE(duplicate_rejected);

    reconciler.update(Component::column({
        Component::row({Component::text("A"),Component::text("BC")},"row",LayoutStyle{.gap = 2}),
        Component::box({Component::text("X")},LayoutStyle{.width = 8,.padding = 1},"box")
    },"root",LayoutStyle{.gap = 1}));
    auto* layout_root = reconciler.root();
    EXPECT_EQ(layout_root->measure(Constraints{.maximum = {.width = 40,.height = 20}}),(Size{.width = 8,.height = 5}));
    layout_root->layout(Constraints{.maximum = {.width = 40,.height = 20}});
    EXPECT_EQ(layout_root->children()[0]->children()[1]->bounds().x,3);
    EXPECT_EQ(layout_root->children()[1]->bounds().y,2);
    EXPECT_EQ(layout_root->children()[1]->children()[0]->bounds().x,1);
    EXPECT_EQ(layout_root->children()[1]->children()[0]->bounds().y,3);
    ScreenBuffer layout_buffer({.width = 12,.height = 8});
    Painter layout_painter(layout_buffer);
    layout_root->paint(layout_painter);
    EXPECT_EQ(layout_buffer.at(3,0).grapheme,"B");
    EXPECT_EQ(layout_buffer.at(1,3).grapheme,"X");
    reconciler.update(Component::row({Component::text("界X"),Component::text("Z")}));
    layout_root = reconciler.root();
    layout_root->layout(Constraints{.maximum = {.width = 2,.height = 1}});
    EXPECT_EQ(layout_root->bounds().width,2);
    EXPECT_EQ(layout_root->children()[0]->bounds().width,2);
    EXPECT_EQ(layout_root->children()[1]->bounds().width,0);
    ScreenBuffer clipped({.width = 2,.height = 1});
    Painter clipped_painter(clipped);
    layout_root->paint(clipped_painter);
    EXPECT_EQ(clipped.at(0,0).grapheme,"界");
    EXPECT_EQ(clipped.at(1,0).width,0);

}
