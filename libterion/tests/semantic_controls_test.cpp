/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <terion/terion.h>
#include <gtest/gtest.h>
#include <array>

using namespace terion;

TEST(SemanticColors, InteractiveControlsUseSemanticAccents)
{
    const Theme theme = Theme::dark();
    Reconciler reconciler;
    reconciler.update(Column({.children = {
        Checkbox({.label = "Checkbox",.checked = true,.color = ColorVariant::Success}),
        Switch({.label = "Switch",.checked = true,.color = ColorVariant::Warning}),
        Radio({.label = "Radio",.checked = true,.color = ColorVariant::Error}),
        RadioGroup({.options = {{.label = "First",.value = "1"}},.value = "1",
            .color = ColorVariant::Info}),
        Slider({.value = 50,.color = ColorVariant::Success})
    }}));
    auto* root = reconciler.root();
    const Size screen{.width = 65,.height = 22};
    root->layout({.maximum = screen});
    ScreenBuffer buffer(screen);
    Painter painter(buffer);
    root->paint(painter,{}, {},std::nullopt,&theme);

    const auto expected = std::array{theme.palette.success,theme.palette.warning,
        theme.palette.error,theme.palette.info,theme.palette.success};
    for (std::size_t i = 0; i < expected.size(); ++i)
    {
        const Rect bounds = root->children()[i]->bounds();
        ASSERT_GT(bounds.width,0);
        EXPECT_EQ(buffer.at(bounds.x,bounds.y).style.foreground,expected[i]);
    }
}

TEST(SemanticColors, DisabledControlsStayMuted)
{
    const Theme theme = Theme::dark();
    Reconciler reconciler;
    reconciler.update(Column({.children = {
        Checkbox({.label = "Checkbox",.checked = true,.color = ColorVariant::Error,.disabled = true}),
        Slider({.value = 50,.disabled = true,.color = ColorVariant::Success})
    }}));
    const Size screen{.width = 65,.height = 10};
    reconciler.root()->layout({.maximum = screen});
    ScreenBuffer buffer(screen);
    Painter painter(buffer);
    reconciler.root()->paint(painter,{}, {},std::nullopt,&theme);
    for (const auto& child : reconciler.root()->children())
        EXPECT_EQ(buffer.at(child->bounds().x,child->bounds().y).style.foreground,
            theme.palette.muted);
}
