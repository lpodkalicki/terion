/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <terion/terion.h>
#include <gtest/gtest.h>

using namespace terion;

TEST(Render, CanvasDrawsBrailleShapes)
{
    render::Canvas canvas(4,8);
    canvas.point(0,0);
    canvas.point(1,3);
    ScreenBuffer canvas_buffer({.width = 2,.height = 2});
    Painter canvas_painter(canvas_buffer);
    canvas.paint(canvas_painter);
    EXPECT_EQ(canvas_buffer.at(0,0).grapheme,"⢁");
    canvas.clear();
    canvas.rectangle({.x = 0,.y = 0,.width = 4,.height = 8});
    canvas.paint(canvas_painter);
    EXPECT_NE(canvas_buffer.at(0,0).grapheme,"⠀");
    canvas.clear();
    canvas.line({.x = 0,.y = 0},{.x = 3,.y = 7});
    canvas.circle({.x = 1,.y = 2},1);
    canvas.paint(canvas_painter);
    EXPECT_NE(canvas_buffer.at(0,0).grapheme,"⠀");

}

TEST(Render, PainterDrawsBordersAndBackground)
{
    ScreenBuffer drawing({.width = 12,.height = 6});
    Painter drawing_painter(drawing);
    drawing_painter.border({.x = 1,.y = 1,.width = 8,.height = 4},BorderStyle::Rounded);
    EXPECT_EQ(drawing.at(1,1).grapheme,"╭");
    EXPECT_EQ(drawing.at(8,1).grapheme,"╮");
    EXPECT_EQ(drawing.at(1,4).grapheme,"╰");
    EXPECT_EQ(drawing.at(8,4).grapheme,"╯");
    drawing_painter.line({.x = 2,.y = 2},{.x = 6,.y = 2});
    EXPECT_EQ(drawing.at(4,2).grapheme,"─");
    drawing_painter.fillBackground({.x = 2,.y = 2,.width = 3,.height = 1},Color::rgb(3,5,7));
    EXPECT_EQ(drawing.at(3,2).style.background,Color::rgb(3,5,7));
}

TEST(Render, ComponentsDrawStyledBordersAndProgress)
{
    Reconciler styled;
    styled.update(Box({.children = {
        Typography({.text = "Hello"}),
        Divider({.layout = {.width = 9}}),
        Progress({.value = 0.5,.layout = {.width = 8}})
    },.layout = {.padding = 1},.border = BorderStyle::Rounded,
        .background = Color::rgb(8,9,10)}));
    styled.root()->layout(Constraints{.maximum = {.width = 20,.height = 10}});
    ScreenBuffer styled_buffer({.width = 20,.height = 10});
    Painter styled_painter(styled_buffer);
    styled.root()->paint(styled_painter);
    EXPECT_EQ(styled_buffer.at(0,0).grapheme,"╭");
    EXPECT_EQ(styled_buffer.at(2,2).grapheme,"H");
    EXPECT_EQ(styled_buffer.at(2,2).style.background,Color::rgb(8,9,10));
    EXPECT_EQ(styled_buffer.at(2,3).grapheme,"─");
    EXPECT_EQ(styled_buffer.at(2,4).grapheme,"█");
    EXPECT_EQ(styled_buffer.at(6,4).grapheme,"░");

}
