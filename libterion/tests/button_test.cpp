/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <terion/terion.h>
#include <gtest/gtest.h>

using namespace terion;

TEST(Button, Behavior)
{
    Reconciler graphic_buttons;
    graphic_buttons.update(Row({.children = {
        Button({.text = "Save",.key = "save",.variant = ButtonVariant::Contained}),
        Button({.text = "Cancel",.key = "cancel",.variant = ButtonVariant::Outlined}),
        Button({.text = "Help",.key = "help",.variant = ButtonVariant::Text}),
        Button({.text = "Disabled",.key = "disabled",.variant = ButtonVariant::Contained,.disabled = true})
    },.layout = {.gap = 1}}));
    graphic_buttons.root()->layout(Constraints{.maximum = {.width = 80,.height = 5}});
    const auto& buttons = graphic_buttons.root()->children();
    EXPECT_EQ(buttons[0]->bounds().height,3);
    EXPECT_EQ(buttons[1]->bounds().height,3);
    ScreenBuffer buttons_buffer({.width = 80,.height = 5});
    Painter buttons_painter(buttons_buffer);
    const Theme dark = Theme::dark();
    graphic_buttons.root()->paint(buttons_painter,{},std::to_string(buttons[0]->id()),std::nullopt,&dark);
    EXPECT_NE(buttons_buffer.at(buttons[0]->bounds().x,1).style.background,dark.palette.primary);
    EXPECT_EQ(buttons_buffer.at(buttons[1]->bounds().x,0).grapheme,"╭");
    EXPECT_NE(buttons_buffer.at(buttons[2]->bounds().x,0).grapheme,"╭");
    EXPECT_EQ(buttons_buffer.at(buttons[3]->bounds().x,1).style.background,dark.palette.surface);
    buttons_buffer.clear();
    graphic_buttons.root()->paint(buttons_painter,std::to_string(buttons[1]->id()),{},std::nullopt,&dark);
    EXPECT_EQ(buttons_buffer.at(buttons[1]->bounds().x,0).grapheme,"┏");

}
