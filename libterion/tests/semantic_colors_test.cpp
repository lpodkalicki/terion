/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <terion/terion.h>
#include <gtest/gtest.h>

using namespace terion;

TEST(SemanticColors, ResolveThemeTokens)
{
    const auto palette = Theme::dark().palette;
    EXPECT_EQ(palette.resolve(ColorVariant::Success),palette.success);
    EXPECT_EQ(palette.resolve(ColorVariant::Warning),palette.warning);
    EXPECT_EQ(palette.resolve(ColorVariant::Error),palette.error);
    EXPECT_EQ(palette.resolve(ColorVariant::Info),palette.info);
    EXPECT_EQ(palette.onColor(ColorVariant::Success),palette.on_success);
    EXPECT_EQ(palette.onColor(ColorVariant::Error),palette.on_error);
    EXPECT_NE(Theme::light().palette.error,palette.error);
}

TEST(SemanticColors, ComponentsUseSemanticVariants)
{
    Reconciler reconciler;
    reconciler.update(ThemeProvider(Theme::dark(),Column({.children = {
        Typography({.text = "Warning",.color = ColorVariant::Warning}),
        Button({.text = "Fail",.variant = ButtonVariant::Contained,.color = ColorVariant::Error}),
        Progress({.value = 0.5,.color = ColorVariant::Success})
    }})));
    auto* root = reconciler.root();
    root->layout({.maximum = {.width = 70,.height = 20}});
    ScreenBuffer buffer({.width = 70,.height = 20});
    Painter painter(buffer);
    const Theme theme = Theme::dark();
    root->paint(painter,{}, {},std::nullopt,&theme);
    const auto& children = root->children();
    EXPECT_EQ(buffer.at(children[0]->bounds().x,children[0]->bounds().y).style.foreground,
        Theme::dark().palette.warning);
    EXPECT_EQ(buffer.at(children[1]->bounds().x+3,children[1]->bounds().y+1).style.background,
        Theme::dark().palette.error);
    EXPECT_EQ(buffer.at(children[2]->bounds().x,children[2]->bounds().y).style.foreground,
        Theme::dark().palette.success);
}
