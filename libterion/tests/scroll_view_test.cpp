/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <terion/terion.h>
#include <gtest/gtest.h>

using namespace terion;

TEST(ScrollView, Behavior)
{
    ScreenBuffer clipped_buffer({.width = 10,.height = 4});
    Painter scoped_painter(clipped_buffer);
    {
        Painter::ClipScope outer(scoped_painter,{.x = 1,.y = 1,.width = 7,.height = 2});
        scoped_painter.text(0,1,"ABCDEFGHIJ");
        EXPECT_EQ(clipped_buffer.at(0,1).grapheme," ");
        EXPECT_EQ(clipped_buffer.at(1,1).grapheme,"B");
        EXPECT_EQ(clipped_buffer.at(7,1).grapheme,"H");
        EXPECT_EQ(clipped_buffer.at(8,1).grapheme," ");
        {
            Painter::ClipScope inner(scoped_painter,{.x = 3,.y = 0,.width = 2,.height = 4});
            scoped_painter.fill({.width = 10,.height = 4},Cell{.grapheme = "X"});
        }
        EXPECT_EQ(clipped_buffer.at(2,1).grapheme,"C");
        EXPECT_EQ(clipped_buffer.at(3,1).grapheme,"X");
        EXPECT_EQ(clipped_buffer.at(4,1).grapheme,"X");
        scoped_painter.put(8,1,"Z");
        EXPECT_EQ(clipped_buffer.at(8,1).grapheme," ");
    }
    scoped_painter.put(8,1,"Z");
    EXPECT_EQ(clipped_buffer.at(8,1).grapheme,"Z");

    int reported_scroll = -1;
    Reconciler scrolling;
    scrolling.update(ScrollView({.children = {
        Typography({.text = "0"}),Typography({.text = "1"}),Typography({.text = "2"}),
        Typography({.text = "3"}),Typography({.text = "4"}),Typography({.text = "5"})
    },.layout = {.width = 5,.height = 3},.scrollOffset = 2,
        .onScroll = [&](int offset) { reported_scroll = offset; }}));
    scrolling.root()->layout(Constraints{.maximum = {.width = 5,.height = 3}});
    ScreenBuffer scroll_buffer({.width = 5,.height = 6});
    Painter scroll_painter(scroll_buffer);
    scrolling.root()->paint(scroll_painter);
    EXPECT_EQ(scroll_buffer.at(0,0).grapheme,"2");
    EXPECT_EQ(scroll_buffer.at(0,2).grapheme,"4");
    EXPECT_EQ(scroll_buffer.at(0,3).grapheme," ");
    EXPECT_EQ(scroll_buffer.at(4,0).grapheme,"│");
    EXPECT_EQ(scroll_buffer.at(4,1).grapheme,"█");
    EXPECT_EQ(scrolling.root()->scrollbarTrack().x,4);
    EXPECT_EQ(scrolling.root()->scrollbarThumb().y,1);
    ScreenBuffer hover_scroll_buffer({.width = 5,.height = 4});
    Painter hover_scroll_painter(hover_scroll_buffer);
    scrolling.root()->paint(hover_scroll_painter,{},std::to_string(scrolling.root()->id()));
    EXPECT_NE(hover_scroll_buffer.at(4,1).style.foreground,scroll_buffer.at(4,1).style.foreground);
    int grab = -1;
    EXPECT_TRUE(scrolling.root()->scrollbarPress({.x = 4,.y = 1},grab));
    EXPECT_EQ(grab,0);
    EXPECT_TRUE(scrolling.root()->scrollbarDrag(2,grab));
    EXPECT_EQ(reported_scroll,3);
    EXPECT_TRUE(scrolling.root()->scrollbarPress({.x = 4,.y = 0},grab));
    EXPECT_EQ(reported_scroll,0);
    EXPECT_EQ(scrolling.root()->scrollTarget({.x = 1,.y = 1}),scrolling.root());
    EXPECT_TRUE(scrolling.root()->scrollBy(3));
    EXPECT_EQ(reported_scroll,3);
    EXPECT_TRUE(scrolling.root()->scrollBy(-100));
    EXPECT_EQ(reported_scroll,0);
    EXPECT_TRUE(scrolling.root()->handleScrollKey(Event{.key = Key::PageDown}));
    EXPECT_EQ(reported_scroll,3);
    EXPECT_TRUE(scrolling.root()->handleScrollKey(Event{.key = Key::Up}));
    EXPECT_EQ(reported_scroll,1);
    EXPECT_TRUE(scrolling.root()->handleScrollKey(Event{.key = Key::Home}));
    EXPECT_EQ(reported_scroll,0);
    EXPECT_TRUE(scrolling.root()->handleScrollKey(Event{.key = Key::End}));
    EXPECT_EQ(reported_scroll,3);
    State<int> wheel_offset{0};
    Application wheel_app(std::make_unique<HeadlessTerminalBackend>(Size{.width = 8,.height = 3}));
    wheel_app.render([&] {
        return ScrollView({.children = {
            Typography({.text = "0"}),Typography({.text = "1"}),Typography({.text = "2"}),
            Typography({.text = "3"}),Typography({.text = "4"}),Typography({.text = "5"})
        },.layout = {.width = 8,.height = 3},.scrollOffset = wheel_offset.get(),
            .onScroll = [&](int next) { wheel_offset.set(next); }});
    });
    wheel_app.renderOnce();
    EXPECT_EQ(wheel_app.focus().current(),std::to_string(wheel_app.reconciler().root()->id()));
    EXPECT_TRUE(wheel_app.dispatchEvent(Event{.key = Key::PageDown}));
    EXPECT_EQ(wheel_offset.get(),2);
    wheel_app.renderOnce();
    EXPECT_TRUE(wheel_app.dispatchEvent(Event{.key = Key::Home}));
    EXPECT_EQ(wheel_offset.get(),0);
    wheel_app.renderOnce();
    EXPECT_TRUE(wheel_app.dispatchEvent(Event{.key = Key::Mouse,.mouse = {
        .position = {.x = 1,.y = 1},.action = MouseAction::Scroll,.scrollY = -1}}));
    EXPECT_EQ(wheel_offset.get(),3);
    wheel_app.renderOnce();
    EXPECT_EQ(wheel_app.reconciler().root()->children()[0]->bounds().y,-3);
    auto& wheel_pointer_output = dynamic_cast<HeadlessTerminalBackend&>(wheel_app.backend());
    wheel_pointer_output.clearOutput();
    wheel_app.dispatchEvent(Event{.key = Key::Mouse,.mouse = {
        .position = {.x = 7,.y = 2},.action = MouseAction::Move}});
    EXPECT_NE(wheel_pointer_output.output().find("\033]22;grab\033\\"),std::string::npos);
    wheel_pointer_output.clearOutput();
    EXPECT_TRUE(wheel_app.dispatchEvent(Event{.key = Key::Mouse,.mouse = {
        .position = {.x = 7,.y = 2},.button = MouseButton::Left,.action = MouseAction::Press}}));
    EXPECT_NE(wheel_pointer_output.output().find("\033]22;grabbing\033\\"),std::string::npos);
    EXPECT_TRUE(wheel_app.dispatchEvent(Event{.key = Key::Mouse,.mouse = {
        .position = {.x = 7,.y = 0},.button = MouseButton::Left,.action = MouseAction::Move}}));
    EXPECT_EQ(wheel_offset.get(),0);
    wheel_app.renderOnce();
    EXPECT_TRUE(wheel_app.dispatchEvent(Event{.key = Key::Mouse,.mouse = {
        .position = {.x = 30,.y = 100},.button = MouseButton::Left,.action = MouseAction::Move}}));
    EXPECT_EQ(wheel_offset.get(),3);
    EXPECT_TRUE(wheel_app.dispatchEvent(Event{.key = Key::Mouse,.mouse = {
        .position = {.x = 30,.y = 100},.button = MouseButton::Left,.action = MouseAction::Release}}));
    wheel_app.renderOnce();
    wheel_app.dispatchEvent(Event{.key = Key::Mouse,.mouse = {
        .position = {.x = 7,.y = 0},.action = MouseAction::Move}});
    EXPECT_EQ(wheel_offset.get(),3);

}
