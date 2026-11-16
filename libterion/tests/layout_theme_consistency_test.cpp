/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <terion/terion.h>
#include <gtest/gtest.h>

using namespace terion;

TEST(LayoutConsistency, DensityMetricsPreserveLegacyDefaults)
{
    const Theme theme = Theme::dark();
    EXPECT_EQ(theme.controlMetrics(ButtonSize::Compact),(ControlMetrics{1,1}));
    EXPECT_EQ(theme.controlMetrics(ButtonSize::Medium),(ControlMetrics{3,2}));
    EXPECT_EQ(theme.controlMetrics(ButtonSize::Large),(ControlMetrics{5,3}));
    Theme compact = theme;
    compact.density = ComponentDensity::Compact;
    EXPECT_EQ(compact.controlMetrics(ButtonSize::Medium).height,1);
    EXPECT_EQ(compact.controlMetrics(ButtonSize::Large).height,3);
    Theme spacious = theme;
    spacious.density = ComponentDensity::Spacious;
    EXPECT_EQ(spacious.controlMetrics(ButtonSize::Medium).height,5);
    EXPECT_EQ(spacious.controlMetrics(ButtonSize::Large).height,7);
    spacious.focus_color = Color::rgb(255,110,0);
    EXPECT_EQ(spacious.focusColor(),Color::rgb(255,110,0));
    EXPECT_EQ(theme.focusColor(),theme.palette.primary);
}

TEST(LayoutConsistency, ResponsiveColumnsRespectMinWidthAndGaps)
{
    EXPECT_EQ(responsiveColumns(80,3,1,24),3);
    EXPECT_EQ(responsiveColumns(60,3,1,24),2);
    EXPECT_EQ(responsiveColumns(20,3,1,24),1);
    EXPECT_EQ(responsiveColumns(0,3,2,24),1);
    EXPECT_EQ(responsiveColumns(20,3,1,0),3); // Legacy fixed columns.
    EXPECT_EQ(responsiveColumns(40,0,1,12),1);
}

TEST(LayoutConsistency, GridReflowsAtNarrowWidths)
{
    Reconciler reconciler;
    const auto makeGrid = [](const int width) {
        return Grid({.children={
            Box({.children={Typography({.text="A"})}}),
            Box({.children={Typography({.text="B"})}}),
            Box({.children={Typography({.text="C"})}})
        },.layout={.width=width,.gap=1},.columns=3,.min_column_width=12});
    };
    reconciler.update(makeGrid(39));
    auto* root = reconciler.root();
    root->layout({.maximum={.width=39,.height=20}});
    EXPECT_EQ(root->children()[0]->bounds().y,root->children()[1]->bounds().y);
    EXPECT_EQ(root->children()[1]->bounds().y,root->children()[2]->bounds().y);
    reconciler.update(makeGrid(25));
    root = reconciler.root();
    root->layout({.maximum={.width=25,.height=20}});
    EXPECT_EQ(root->children()[0]->bounds().y,root->children()[1]->bounds().y);
    EXPECT_GT(root->children()[2]->bounds().y,root->children()[1]->bounds().y);
    reconciler.update(makeGrid(12));
    root = reconciler.root();
    root->layout({.maximum={.width=12,.height=20}});
    EXPECT_LT(root->children()[0]->bounds().y,root->children()[1]->bounds().y);
    EXPECT_LT(root->children()[1]->bounds().y,root->children()[2]->bounds().y);
}

TEST(LayoutConsistency, ThemeUpdatesControlGeometryWithoutReplacingComponent)
{
    Application app(std::make_unique<HeadlessTerminalBackend>(Size{40,20}));
    app.render([] {return Button({.text="Action",.key="action"});});
    app.renderOnce();
    const auto id = app.reconciler().root()->id();
    EXPECT_EQ(app.reconciler().root()->bounds().height,3);
    Theme theme = Theme::dark();
    theme.density = ComponentDensity::Compact;
    app.setTheme(theme);
    app.renderOnce();
    EXPECT_EQ(app.reconciler().root()->id(),id);
    EXPECT_EQ(app.reconciler().root()->bounds().height,1);
    theme.density = ComponentDensity::Spacious;
    app.setTheme(theme);
    app.renderOnce();
    EXPECT_EQ(app.reconciler().root()->bounds().height,5);
}

TEST(LayoutConsistency, SpacingTokensFollowDensityButLiteralValuesStayFixed)
{
    Reconciler reconciler;
    reconciler.update(Column({.children={
        Typography({.text="Top"}),Typography({.text="Bottom"})
    },.layout={.gap=9,.gap_token=SpacingToken::Medium}}));
    auto* root = reconciler.root();
    root->layout({.maximum={.width=40,.height=30}});
    EXPECT_EQ(root->children()[1]->bounds().y,3);
    Theme theme = Theme::dark();
    theme.density = ComponentDensity::Spacious;
    root->setLayoutTheme(theme);
    root->layout({.maximum={.width=40,.height=30}});
    EXPECT_EQ(root->children()[1]->bounds().y,4);
    reconciler.update(Column({.children={
        Typography({.text="Top"}),Typography({.text="Bottom"})
    },.layout={.gap=5}}));
    root = reconciler.root();
    root->setLayoutTheme(theme);
    root->layout({.maximum={.width=40,.height=30}});
    EXPECT_EQ(root->children()[1]->bounds().y,6);
}
