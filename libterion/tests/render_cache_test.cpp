/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <gtest/gtest.h>
#include <terion/terion.h>

using namespace terion;

TEST(RenderCache, ReusesBaseWhenTreeIsUnchanged)
{
    Application app(std::make_unique<HeadlessTerminalBackend>(Size{.width = 32,.height = 8}));
    app.render([] { return Column({.children = {Typography({.text = "Ready",.key = "status"})}}); });
    app.renderOnce();
    EXPECT_EQ(app.renderStats().base_paints,1u);
    EXPECT_EQ(app.renderStats().cache_hits,0u);
    app.renderOnce();
    EXPECT_EQ(app.renderStats().base_paints,1u);
    EXPECT_EQ(app.renderStats().cache_hits,1u);
}

TEST(RenderCache, InvalidatesOnTextAndThemeChanges)
{
    Application app(std::make_unique<HeadlessTerminalBackend>(Size{.width = 32,.height = 8}));
    std::string label = "First";
    app.render([&] { return Typography({.text = label}); });
    app.renderOnce();
    app.renderOnce();
    EXPECT_EQ(app.renderStats().cache_hits,1u);
    label = "Updated";
    app.renderOnce();
    EXPECT_EQ(app.renderStats().base_paints,2u);
    app.setTheme(Theme::light());
    app.renderOnce();
    EXPECT_EQ(app.renderStats().base_paints,3u);
}

TEST(RenderCache, DoesNotCacheAnimatedProgress)
{
    Application app(std::make_unique<HeadlessTerminalBackend>(Size{.width = 32,.height = 8}));
    app.render([] { return Progress({.indeterminate = true}); });
    app.renderOnce();
    app.renderOnce();
    EXPECT_EQ(app.renderStats().base_paints,2u);
    EXPECT_EQ(app.renderStats().cache_hits,0u);
}

TEST(RenderCache, PreservesFocusRenderingAfterChange)
{
    Application app(std::make_unique<HeadlessTerminalBackend>(Size{.width = 32,.height = 8}));
    app.render([] { return Column({.children = {
        Typography({.text = "Caption"}),
        Button({.text = "One",.key = "one",.onClick = [] {}}),
        Button({.text = "Two",.key = "two",.onClick = [] {}})
    }}); });
    app.renderOnce();
    EXPECT_TRUE(app.focus("two"));
    app.renderOnce();
    EXPECT_EQ(app.renderStats().base_paints,2u);
}
