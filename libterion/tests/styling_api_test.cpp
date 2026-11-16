/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */
#include <gtest/gtest.h>
#include <terion/terion.h>
#include <terion/components/text_view_detail.h>

using namespace terion;

TEST(StylingApi, ResolvesSemanticAndRgbColorsAtPaintTime)
{
    const TextStyleSpec spec{
        .foreground = ColorVariant::Success,
        .background = Color::rgb(13,24,35),
        .bold = true,.italic = true,.underline = true,
        .strikethrough = true,.dim = true
    };
    const auto dark = spec.resolve(Theme::dark().palette);
    const auto light = spec.resolve(Theme::light().palette);
    EXPECT_EQ(dark.foreground,Theme::dark().palette.success);
    EXPECT_EQ(light.foreground,Theme::light().palette.success);
    EXPECT_NE(dark.foreground,light.foreground);
    EXPECT_EQ(dark.background,Color::rgb(13,24,35));
    EXPECT_EQ(light.background,Color::rgb(13,24,35));
    EXPECT_TRUE(dark.bold);
    EXPECT_TRUE(dark.italic);
    EXPECT_TRUE(dark.underline);
    EXPECT_TRUE(dark.strikethrough);
    EXPECT_TRUE(dark.dim);
    EXPECT_EQ(TextStyleSpec{.background = ColorVariant::Default}.resolve(Theme::light().palette).background,
        Theme::light().palette.background);
}

TEST(StylingApi, OptionalFlagsPreserveOrOverrideInheritedStyles)
{
    const TextStyle base{.foreground = Color::rgb(3,5,7),.background = Color::rgb(11,13,17),
        .bold = true,.underline = true,.italic = true};
    const auto result = TextStyleSpec{.bold = false,.dim = true}.resolve(Theme::dark().palette,base);
    EXPECT_FALSE(result.bold);
    EXPECT_TRUE(result.italic);
    EXPECT_TRUE(result.underline);
    EXPECT_TRUE(result.dim);
    EXPECT_EQ(result.foreground,base.foreground);
    EXPECT_EQ(result.background,base.background);
}

TEST(StylingApi, TypographyRespondsToThemeChangesWithoutRebuildingSpec)
{
    Reconciler tree;
    const auto component = [] { return Typography({.text = "Theme",.key = "color-label",
        .style_spec = TextStyleSpec{.foreground = ColorVariant::Warning,.underline = true}}); };
    tree.update(component());
    tree.root()->layout({.maximum = {.width = 30,.height = 3}});
    ScreenBuffer dark({.width = 30,.height = 3});
    Painter painter_dark(dark);
    const Theme dark_theme = Theme::dark();
    tree.root()->paint(painter_dark,{}, {},std::nullopt,&dark_theme);
    EXPECT_EQ(dark.at(0,0).style.foreground,dark_theme.palette.warning);
    EXPECT_TRUE(dark.at(0,0).style.underline);

    tree.update(component());
    tree.root()->layout({.maximum = {.width = 30,.height = 3}});
    ScreenBuffer light({.width = 30,.height = 3});
    Painter painter_light(light);
    const Theme light_theme = Theme::light();
    tree.root()->paint(painter_light,{}, {},std::nullopt,&light_theme);
    EXPECT_EQ(light.at(0,0).style.foreground,light_theme.palette.warning);
    EXPECT_NE(dark.at(0,0).style.foreground,light.at(0,0).style.foreground);
}

TEST(StylingApi, StyledTextKeepsDifferentRgbAndSpecSpansDistinct)
{
    StyledText spans;
    detail::appendSpan(spans,{.text = "A",.foreground = Color::rgb(255,0,0)});
    detail::appendSpan(spans,{.text = "B",.foreground = Color::rgb(0,255,0)});
    detail::appendSpan(spans,{.text = "C",.style_spec = TextStyleSpec{.foreground = ColorVariant::Info}});
    detail::appendSpan(spans,{.text = "D",.style_spec = TextStyleSpec{.foreground = ColorVariant::Error}});
    ASSERT_EQ(spans.size(),4u);
    const auto wrapped = detail::wrapStyled(spans,2);
    ASSERT_EQ(wrapped.size(),2u);
    ASSERT_EQ(wrapped[0].size(),2u);
    ASSERT_EQ(wrapped[1].size(),2u);
    EXPECT_EQ(wrapped[1][0].style_spec->foreground->resolveForeground(Theme::dark().palette),
        Theme::dark().palette.info);
}

TEST(StylingApi, StyledVirtualListUsesSemanticForegroundAndRgbBackground)
{
    Reconciler tree;
    const Color custom = Color::rgb(16,38,72);
    tree.update(VirtualList({.item_count = 1,
        .item_styled = [custom](int) { return StyledText{{.text = "OK",
            .style_spec = TextStyleSpec{.foreground = ColorVariant::Success,
                .background = custom,.bold = true}}}; },
        .layout = {.width = 8,.height = 2},.show_scrollbar = false}));
    tree.root()->layout({.maximum = {.width = 8,.height = 2}});
    ScreenBuffer screen({.width = 8,.height = 2});
    Painter painter(screen);
    const Theme theme = Theme::dark();
    tree.root()->paint(painter,{}, {},std::nullopt,&theme);
    EXPECT_EQ(screen.at(0,0).grapheme,"O");
    EXPECT_EQ(screen.at(0,0).style.foreground,theme.palette.success);
    EXPECT_EQ(screen.at(0,0).style.background,custom);
    EXPECT_TRUE(screen.at(0,0).style.bold);
    EXPECT_EQ(screen.at(1,0).style.background,custom);
}

TEST(StylingApi, BothTablesHonorDeclarativeCellStyling)
{
    const Color custom = Color::rgb(17,33,55);
    const auto formatter = [custom](const TableRow&,std::size_t) {
        return TableCellFormat{.text = "OK",.style_spec = TextStyleSpec{
            .foreground = ColorVariant::Warning,.background = custom,.italic = true}};
    };
    const auto verify = [&](Component component,int x,int y) {
        Reconciler tree;
        tree.update(std::move(component));
        tree.root()->layout({.maximum = {.width = 15,.height = 4}});
        ScreenBuffer screen({.width = 15,.height = 4});
        Painter painter(screen);
        const Theme theme = Theme::dark();
        tree.root()->paint(painter,{}, {},std::nullopt,&theme);
        const auto& cell = screen.at(x,y);
        EXPECT_EQ(cell.style.foreground,theme.palette.warning);
        EXPECT_EQ(cell.style.background,custom);
        EXPECT_TRUE(cell.style.italic);
    };
    verify(Table({.columns = {{.title = "Status",.width = 12}},
        .rows = {{.key = "1",.cells = {"OK"}}},.format_cell = formatter,
        .layout = {.width = 15,.height = 4}}),1,2);
    verify(VirtualTable({.columns = {{.title = "Status",.width = 12}},
        .row_count = 1,.row_at = [](int) { return TableRow{.key = "1",.cells = {"OK"}}; },
        .format_cell = formatter,.layout = {.width = 15,.height = 4},
        .show_scrollbar = false}),0,1);
}

TEST(StylingApi, LegacyRgbStylingContinuesToWork)
{
    const Color rgb = Color::rgb(45,132,211);
    Reconciler tree;
    tree.update(Typography({.text = "Legacy",.style = {.foreground = rgb}}));
    tree.root()->layout({.maximum = {.width = 20,.height = 2}});
    ScreenBuffer screen({.width = 20,.height = 2});
    Painter painter(screen);
    tree.root()->paint(painter);
    EXPECT_EQ(screen.at(0,0).style.foreground,rgb);
}

TEST(StylingApi, ButtonAndDividerUseTheSameSemanticColorSpec)
{
    const auto warning = TextStyleSpec{.foreground = ColorVariant::Warning};
    const auto theme = Theme::light();
    Reconciler divider;
    divider.update(Divider({.layout = {.width = 6},.style_spec = warning}));
    divider.root()->layout({.maximum = {.width = 6,.height = 1}});
    ScreenBuffer lines({.width = 6,.height = 1});
    Painter draw_lines(lines);
    divider.root()->paint(draw_lines,{}, {},std::nullopt,&theme);
    EXPECT_EQ(lines.at(0,0).style.foreground,theme.palette.warning);

    const Color custom = Color::rgb(30,40,50);
    Reconciler button;
    button.update(Button({.text = "Go",.variant = ButtonVariant::Text,
        .layout = {.width = 6,.height = 1},.size = ControlSize::Compact,
        .style_spec = TextStyleSpec{.foreground = ColorVariant::Warning,
            .background = custom,.italic = true}}));
    button.root()->layout({.maximum = {.width = 6,.height = 1}});
    ScreenBuffer screen({.width = 6,.height = 1});
    Painter painter(screen);
    button.root()->paint(painter,{}, {},std::nullopt,&theme);
    bool seen = false;
    for (int x = 0; x < 6; ++x)
        if (screen.at(x,0).grapheme == "G")
        {
            seen = true;
            EXPECT_EQ(screen.at(x,0).style.foreground,theme.palette.warning);
            EXPECT_EQ(screen.at(x,0).style.background,custom);
            EXPECT_TRUE(screen.at(x,0).style.italic);
        }
    EXPECT_TRUE(seen);
}

TEST(StylingApi, TextInputsApplyAppearanceWithoutChangingFocusBehavior)
{
    const auto theme = Theme::dark();
    const auto appearance = TextStyleSpec{.foreground = ColorVariant::Info,
        .background = Color::rgb(20,30,40),.italic = true};
    Reconciler field;
    field.update(TextField({.value = "ABC",.layout = {.width = 10,.height = 3},
        .style_spec = appearance}));
    field.root()->layout({.maximum = {.width = 10,.height = 3}});
    ScreenBuffer field_screen({.width = 10,.height = 3});
    Painter field_painter(field_screen);
    field.root()->paint(field_painter,{}, {},std::nullopt,&theme);
    EXPECT_EQ(field_screen.at(2,1).grapheme,"A");
    EXPECT_EQ(field_screen.at(2,1).style.foreground,theme.palette.info);
    EXPECT_EQ(field_screen.at(2,1).style.background,Color::rgb(20,30,40));
    EXPECT_TRUE(field_screen.at(2,1).style.italic);

    Reconciler area;
    area.update(TextArea({.value = "ABC",.layout = {.width = 10,.height = 4},
        .style_spec = appearance}));
    area.root()->layout({.maximum = {.width = 10,.height = 4}});
    ScreenBuffer area_screen({.width = 10,.height = 4});
    Painter area_painter(area_screen);
    area.root()->paint(area_painter,{}, {},std::nullopt,&theme);
    EXPECT_EQ(area_screen.at(1,1).grapheme,"A");
    EXPECT_EQ(area_screen.at(1,1).style.foreground,theme.palette.info);
    EXPECT_EQ(area_screen.at(1,1).style.background,Color::rgb(20,30,40));
    EXPECT_TRUE(area_screen.at(1,1).style.italic);
}
