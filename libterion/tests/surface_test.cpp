/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <terion/terion.h>
#include <gtest/gtest.h>

using namespace terion;

TEST(Surface, RendersPaperAndCardVariants)
{
    const Theme paper_theme = Theme::light();
    Reconciler surfaces;
    surfaces.update(Stack({.children = {
        Paper({.children = {Typography({.text = "Paper"})},
            .layout = {.width = 14,.padding = 1}}),
        Card({.title = "System",.children = {Typography({.text = "Ready"})},
            .layout = {.width = 14,.padding = 1,.gap = 1}})
    }}));
    surfaces.root()->layout(Constraints{.maximum = {.width = 20,.height = 12}});
    ScreenBuffer surface_buffer({.width = 20,.height = 12});
    Painter surface_painter(surface_buffer);
    surfaces.root()->paint(surface_painter,{},{},std::nullopt,&paper_theme);
    EXPECT_EQ(surface_buffer.at(0,0).style.background,paper_theme.palette.surface);
    EXPECT_EQ(surface_buffer.at(1,1).grapheme,"P");
    const auto card_y = surfaces.root()->children()[1]->bounds().y;
    EXPECT_EQ(surface_buffer.at(0,card_y).grapheme,"╭");
    EXPECT_EQ(surface_buffer.at(2,card_y+2).grapheme,"S");
    EXPECT_EQ(surface_buffer.at(2,card_y+4).grapheme,"R");
    EXPECT_EQ(surface_buffer.at(2,card_y+2).style.background,paper_theme.palette.surface);
    surfaces.update(Paper({.children = {Typography({.text = "Custom"})},
        .variant = PaperVariant::Outlined,.background = Color::rgb(7,8,9)}));
    surfaces.root()->layout(Constraints{.maximum = {.width = 20,.height = 6}});
    surface_buffer.clear();
    surfaces.root()->paint(surface_painter,{},{},std::nullopt,&paper_theme);
    EXPECT_EQ(surface_buffer.at(0,0).grapheme,"┌");
    EXPECT_EQ(surface_buffer.at(2,2).style.background,Color::rgb(7,8,9));

}
