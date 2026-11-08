/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <terion/terion.h>
#include <gtest/gtest.h>

using namespace terion;

TEST(Theme, Behavior)
{
    const Theme light = Theme::light();
    EXPECT_EQ(light.spacing(3),3);
    EXPECT_EQ(light.palette.background,Color::rgb(248,249,251));
    Reconciler themed;
    themed.update(Component::column({
        Button({.text = "OK",.variant = ButtonVariant::Contained,.size = ButtonSize::Compact}),
        Progress({.value = 0.5,.layout = {.width = 4}})
    }));
    themed.root()->layout(Constraints{.maximum = {.width = 15,.height = 4}});
    ScreenBuffer theme_buffer({.width = 15,.height = 4});
    Painter theme_painter(theme_buffer);
    themed.root()->paint(theme_painter,{},{},std::nullopt,&light);
    EXPECT_EQ(theme_buffer.at(0,0).style.background,light.palette.primary);
    EXPECT_EQ(theme_buffer.at(0,0).style.foreground,light.palette.on_primary);
    EXPECT_EQ(theme_buffer.at(0,1).style.foreground,light.palette.primary);
    EXPECT_EQ(theme_buffer.at(3,1).style.foreground,light.palette.muted);

}
