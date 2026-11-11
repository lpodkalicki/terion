/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <gtest/gtest.h>
#include <terion/terion.h>

using namespace terion;

TEST(ResizableSplit, RequiresExactlyTwoChildren)
{
    EXPECT_THROW(ResizableSplit({}),std::invalid_argument);
    EXPECT_THROW(ResizableSplit({.children = {Typography({.text = "One"})}}),std::invalid_argument);
}

TEST(ResizableSplit, HorizontalLayoutAndMouseResizeRespectMinimumSizes)
{
    int changed = -1;
    Reconciler tree;
    const auto build = [&](int position) {
        tree.update(ResizableSplit({.children = {Typography({.text = "Left"}),
            Typography({.text = "Right"})},.position = position,.min_first = 5,.min_second = 6,
            .layout = {.width = 30,.height = 5},.on_change = [&](int value) { changed = value; }}));
        tree.root()->layout({.maximum = {.width = 30,.height = 5}});
    };
    build(10);
    auto* split = tree.root();
    const Rect horizontal_handle = split->split_handle();
    EXPECT_EQ(horizontal_handle.x,10);
    EXPECT_EQ(horizontal_handle.y,0);
    EXPECT_EQ(horizontal_handle.width,1);
    EXPECT_EQ(horizontal_handle.height,5);
    EXPECT_EQ(split->children()[0]->bounds().width,10);
    EXPECT_EQ(split->children()[1]->bounds().x,11);
    EXPECT_EQ(split->split_target({.x = 10,.y = 2}),split);
    EXPECT_EQ(split->split_target({.x = 9,.y = 2}),nullptr);
    EXPECT_TRUE(split->dragSplit({.x = 29,.y = 2}));
    EXPECT_EQ(changed,23);
    build(changed);
    EXPECT_TRUE(tree.root()->handleSplitKey({.key = Key::Left}));
    EXPECT_EQ(changed,22);
    EXPECT_TRUE(tree.root()->handleSplitKey({.key = Key::Home}));
    EXPECT_EQ(changed,5);
}

TEST(ResizableSplit, VerticalLayoutAndKeyboardResize)
{
    int changed = -1;
    Reconciler tree;
    tree.update(ResizableSplit({.children = {Typography({.text = "Top"}),
        Typography({.text = "Bottom"})},.direction = SplitDirection::Vertical,
        .position = 4,.min_first = 2,.min_second = 3,
        .layout = {.width = 20,.height = 12},
        .on_change = [&](int value) { changed = value; }}));
    tree.root()->layout({.maximum = {.width = 20,.height = 12}});
    const Rect vertical_handle = tree.root()->split_handle();
    EXPECT_EQ(vertical_handle.x,0);
    EXPECT_EQ(vertical_handle.y,4);
    EXPECT_EQ(vertical_handle.width,20);
    EXPECT_EQ(vertical_handle.height,1);
    EXPECT_EQ(tree.root()->children()[1]->bounds().y,5);
    EXPECT_TRUE(tree.root()->handleSplitKey({.key = Key::Down}));
    EXPECT_EQ(changed,5);
    EXPECT_TRUE(tree.root()->dragSplit({.x = 1,.y = 100}));
    EXPECT_EQ(changed,8);
}

TEST(ResizableSplit, PaintsDividerInItsOwnCell)
{
    Reconciler tree;
    tree.update(ResizableSplit({.children = {Typography({.text = "Left"}),
        Typography({.text = "Right"})},.position = 6,
        .layout = {.width = 20,.height = 3}}));
    tree.root()->layout({.maximum = {.width = 20,.height = 3}});
    ScreenBuffer screen({.width = 20,.height = 3});
    Painter painter(screen);
    tree.root()->paint(painter);
    EXPECT_EQ(screen.at(6,0).grapheme,"│");
    EXPECT_EQ(screen.at(7,0).grapheme,"R");
}

TEST(ResizableSplit, ApplicationDragsDividerAndKeepsNewPosition)
{
    Application app(std::make_unique<HeadlessTerminalBackend>(Size{40,8}));
    State<int> position{10};
    app.render([&] {
        return ResizableSplit({.children = {Typography({.text = "Left"}),
            Typography({.text = "Right"})},.position = position.get(),
            .layout = {.width = 40,.height = 8},
            .on_change = [&](int value) { position.set(value); }});
    });
    app.renderOnce();
    EXPECT_TRUE(app.dispatchEvent(Event{.key = Key::Mouse,.mouse = {
        .position = {.x = 10,.y = 3},.button = MouseButton::Left,.action = MouseAction::Press}}));
    EXPECT_TRUE(app.dispatchEvent(Event{.key = Key::Mouse,.mouse = {
        .position = {.x = 18,.y = 3},.button = MouseButton::Left,.action = MouseAction::Move}}));
    EXPECT_EQ(position.get(),18);
    app.renderOnce();
    EXPECT_EQ(app.reconciler().root()->split_handle().x,18);
    EXPECT_TRUE(app.dispatchEvent(Event{.key = Key::Mouse,.mouse = {
        .position = {.x = 18,.y = 3},.button = MouseButton::Left,.action = MouseAction::Release}}));
    EXPECT_TRUE(app.dispatchEvent(Event{.key = Key::Right}));
    EXPECT_EQ(position.get(),19);
}
