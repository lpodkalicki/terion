/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <terion/terion.h>
#include <gtest/gtest.h>

using namespace terion;

TEST(ThemeProvider, CascadesToDescendantsAndIsolatesNestedProviders)
{
    Reconciler reconciler;
    reconciler.update(Column({.children = {
        ThemeProvider(Theme::ocean(),Column({.children = {
            Typography({.text = "Ocean"}),
            ThemeProvider(Theme::forest(),Typography({.text = "Forest"})),
            Typography({.text = "Ocean again"})
        }})),
        Typography({.text = "Default"})
    }}));
    const auto* root = reconciler.root();
    const auto* ocean = root->children()[0].get();
    EXPECT_EQ(ocean->resolvedTheme(Theme::dark()),Theme::ocean());
    ASSERT_EQ(ocean->children().size(),3u);
    EXPECT_EQ(ocean->children()[0]->resolvedTheme(Theme::dark()),Theme::ocean());
    EXPECT_EQ(ocean->children()[1]->resolvedTheme(Theme::dark()),Theme::forest());
    EXPECT_EQ(ocean->children()[2]->resolvedTheme(Theme::dark()),Theme::ocean());
    EXPECT_EQ(root->children()[1]->resolvedTheme(Theme::dark()),Theme::dark());
}

TEST(ThemeProvider, UpdatesThemeWithoutReplacingNodes)
{
    Reconciler reconciler;
    reconciler.update(ThemeProvider(Theme::ocean(),Typography({.text = "Label"})));
    const auto id = reconciler.root()->id();
    reconciler.update(ThemeProvider(Theme::violet(),Typography({.text = "Label"})));
    EXPECT_EQ(reconciler.root()->id(),id);
    EXPECT_TRUE(reconciler.root()->paintDirty());
    EXPECT_EQ(reconciler.root()->resolvedTheme(Theme::dark()),Theme::violet());
}

TEST(ThemeProvider, MakesThemeAvailableToFunctionComponents)
{
    const auto label = component([] {
        const Theme theme = useTheme();
        return Typography({.text = std::to_string(theme.palette.primary.red)});
    });
    Reconciler reconciler;
    reconciler.update(ThemeProvider(Theme::sunset(),label()));
    EXPECT_EQ(reconciler.root()->text(),"255");
}

TEST(ThemeProvider, ApplicationThemeIsInheritedWithoutExplicitProvider)
{
    auto backend = std::make_unique<HeadlessTerminalBackend>(Size{.width = 30,.height = 4});
    Application app(std::move(backend));
    const auto label = component([] {
        const Theme theme = useTheme();
        return Typography({.text = std::to_string(theme.palette.primary.red)});
    });
    app.setTheme(Theme::sunset());
    app.render([&] { return label(); });
    app.renderOnce();
    ASSERT_NE(app.reconciler().root(),nullptr);
    EXPECT_EQ(app.reconciler().root()->text(),
        std::to_string(Theme::sunset().palette.primary.red));
}

TEST(ThemeProvider, HasNineDistinctPresets)
{
    const std::array presets{Theme::dark(),Theme::light(),Theme::ocean(),Theme::forest(),
        Theme::sunset(),Theme::violet(),Theme::nord(),Theme::dracula(),Theme::solarized()};
    for (std::size_t i = 0; i < presets.size(); ++i)
        for (std::size_t j = i+1; j < presets.size(); ++j)
            EXPECT_NE(presets[i],presets[j]);
}
