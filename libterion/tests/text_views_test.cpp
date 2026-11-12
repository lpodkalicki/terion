/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <gtest/gtest.h>
#include <terion/terion.h>

using namespace terion;

TEST(MarkdownView, RendersHeadingsListsCodeAndLinks)
{
    const auto lines = MarkdownView::render("# Heading\n- **bold** item\n> quote\n```cpp\nint x;\n```\n[site](https://example.org)");
    ASSERT_EQ(lines.size(),7u);
    EXPECT_EQ(lines[0][1].text,"Heading");
    EXPECT_TRUE(lines[0][1].bold);
    EXPECT_EQ(lines[1][0].text,"  • ");
    EXPECT_EQ(lines[1][1].text,"bold");
    EXPECT_TRUE(lines[1][1].bold);
    EXPECT_EQ(lines[2][0].text,"│ ");
    EXPECT_EQ(lines[4][0].text,"  ");
    EXPECT_EQ(lines[4][1].text,"int");
    EXPECT_EQ(lines[4][1].color,ColorVariant::Primary);
    EXPECT_EQ(lines[6][0].text,"site");
    EXPECT_EQ(lines[6][0].color,ColorVariant::Primary);
}

TEST(MarkdownView, HandlesBlankLinesAndWindowsNewlines)
{
    const auto lines = MarkdownView::render("one\r\n\r\ntwo\r\n");
    ASSERT_EQ(lines.size(),3u);
    EXPECT_EQ(lines[0][0].text,"one");
    EXPECT_TRUE(lines[1].empty());
    EXPECT_EQ(lines[2][0].text,"two");
}

TEST(DiffView, ColorsChangesAndCountsHunkLines)
{
    const auto lines = DiffView::render("@@ -10,2 +20,3 @@\n old\n-removed\n+new\n+another\n");
    ASSERT_EQ(lines.size(),5u);
    EXPECT_EQ(lines[0][0].color,ColorVariant::Info);
    EXPECT_EQ(lines[1][0].text,"   10    20 │ ");
    EXPECT_EQ(lines[2][0].text,"   11       │ ");
    EXPECT_EQ(lines[2][1].color,ColorVariant::Error);
    EXPECT_EQ(lines[3][0].text,"         21 │ ");
    EXPECT_EQ(lines[3][1].color,ColorVariant::Success);
    EXPECT_EQ(lines[4][0].text,"         22 │ ");
}

TEST(DiffView, RecognizesHeadersWithoutConfusingWithChanges)
{
    const auto lines = DiffView::render("--- a/old\n+++ b/new\n@@ -1 +1 @@\n-old\n+new");
    ASSERT_EQ(lines.size(),5u);
    EXPECT_EQ(lines[0][0].color,ColorVariant::Primary);
    EXPECT_EQ(lines[1][0].color,ColorVariant::Primary);
    EXPECT_EQ(lines[3][0].color,ColorVariant::Error);
    EXPECT_EQ(lines[4][0].color,ColorVariant::Success);
}

TEST(TextViews, RenderThroughVirtualList)
{
    Reconciler markdown;
    markdown.update(MarkdownView({.markdown = "# Heading\n- item",
        .layout = {.width = 30,.height = 2}}));
    markdown.root()->layout({.maximum = {.width = 30,.height = 2}});
    ScreenBuffer screen({.width = 30,.height = 2});
    Painter painter(screen);
    markdown.root()->paint(painter);
    EXPECT_EQ(screen.at(0,0).grapheme,"▌");
    Reconciler diff;
    diff.update(DiffView({.diff = "@@ -1 +1 @@\n-old\n+new",
        .show_line_numbers = false,.layout = {.width = 30,.height = 3}}));
    diff.root()->layout({.maximum = {.width = 30,.height = 3}});
    ScreenBuffer diff_screen({.width = 30,.height = 3});
    Painter diff_painter(diff_screen);
    diff.root()->paint(diff_painter);
    EXPECT_EQ(diff_screen.at(0,1).grapheme,"-");
    EXPECT_EQ(diff_screen.at(0,2).grapheme,"+");
}

TEST(MarkdownView, WrapsUtf8WithoutSplittingStyledSpans)
{
    const auto lines = MarkdownView::render("- **Polska** Warszawa Łódź",12);
    ASSERT_GE(lines.size(),2u);
    EXPECT_EQ(lines[0][0].text,"  • ");
    EXPECT_TRUE(lines[0][1].bold);
    EXPECT_EQ(lines[0][1].text,"Polska");
    EXPECT_EQ(lines[1][0].text,"    ");
    for (const auto& line : lines)
        for (const auto& span : line)
            EXPECT_TRUE(isValidUtf8(span.text));
}

TEST(MarkdownView, ColorsKeywordsStringsAndComments)
{
    const auto lines = MarkdownView::render("```cpp\nint x = 42; // note\n```\n");
    ASSERT_EQ(lines.size(),3u);
    EXPECT_EQ(lines[1][1].text,"int");
    EXPECT_EQ(lines[1][1].color,ColorVariant::Primary);
    bool saw_number = false;
    bool saw_comment = false;
    for (const auto& span : lines[1])
    {
        if (span.text == "42" && span.color == ColorVariant::Warning) saw_number = true;
        if (span.text == "// note" && span.color == ColorVariant::Success) saw_comment = true;
    }
    EXPECT_TRUE(saw_number);
    EXPECT_TRUE(saw_comment);
}

TEST(MarkdownView, SupportsInlineEmphasisAndCleanCode)
{
    const auto lines = MarkdownView::render("*em* ~~deleted~~ `code`");
    ASSERT_EQ(lines.size(),1u);
    ASSERT_EQ(lines[0].size(),5u);
    EXPECT_EQ(lines[0][0].text,"em");
    EXPECT_EQ(lines[0][0].color,ColorVariant::Info);
    EXPECT_EQ(lines[0][2].text,"deleted");
    EXPECT_EQ(lines[0][2].color,ColorVariant::Warning);
    EXPECT_EQ(lines[0][4].text,"code");
}

TEST(DiffView, OptionalWrappingPreservesChangeColors)
{
    const auto lines = DiffView::render("@@ -1 +1 @@\n+an exceptionally long addition",false,12);
    ASSERT_GE(lines.size(),3u);
    for (std::size_t i = 1; i < lines.size(); ++i)
        for (const auto& span : lines[i])
            EXPECT_EQ(span.color,ColorVariant::Success);
}

TEST(MarkdownView, RendersTablesWithAlignedCellsAndHeaderStyling)
{
    const auto lines = MarkdownView::render("| Name | Value |\n| :--- | ---: |\n| Ada | 42 |\n| Bob | 7 |");
    ASSERT_EQ(lines.size(),4u);
    EXPECT_EQ(lines[0][0].text,"│ ");
    EXPECT_EQ(lines[0][1].text,"Name");
    EXPECT_TRUE(lines[0][1].bold);
    EXPECT_EQ(lines[1][0].text.substr(0,3),"├");
    EXPECT_NE(lines[2][1].text.find("Ada"),std::string::npos);
    EXPECT_EQ(detail::textCells([&] { std::string text; for (const auto& span : lines[2]) text += span.text; return text; }()),
              detail::textCells([&] { std::string text; for (const auto& span : lines[3]) text += span.text; return text; }()));
}

TEST(MarkdownView, NestedListIndentationAndWrapping)
{
    const auto lines = MarkdownView::render("- parent\n  - child with longer content\n    1. grandchild",14);
    ASSERT_GE(lines.size(),4u);
    EXPECT_EQ(lines[0][0].text,"  • ");
    EXPECT_EQ(lines[1][0].text,"    • ");
    bool numbered = false;
    for (const auto& line : lines)
        if (!line.empty() && line[0].text == "      1. ") numbered = true;
    EXPECT_TRUE(numbered);
}

TEST(MarkdownView, FencedCodeKeepsSyntaxHighlighting)
{
    const auto lines = MarkdownView::render("```cpp\nconst int n = 12;\n```",60);
    ASSERT_EQ(lines.size(),3u);
    EXPECT_EQ(lines[1][1].text,"const");
    EXPECT_EQ(lines[1][1].color,ColorVariant::Primary);
}

TEST(MarkdownView, ReflowsOnResizeWithoutLosingContent)
{
    Reconciler view;
    view.update(MarkdownView({.markdown = "first line with long long words\nsecond line with long long words\nthird line",
        .layout = {.height = 4}}));
    view.root()->layout({.maximum = {.width = 35,.height = 4}});
    ScreenBuffer wide({.width = 35,.height = 4});
    Painter wide_painter(wide);
    view.root()->paint(wide_painter);
    EXPECT_EQ(wide.at(0,0).grapheme,"f");
    view.root()->layout({.maximum = {.width = 15,.height = 4}});
    ScreenBuffer narrow({.width = 15,.height = 4});
    Painter narrow_painter(narrow);
    view.root()->paint(narrow_painter);
    EXPECT_EQ(narrow.at(0,0).grapheme,"f");
    EXPECT_EQ(narrow.at(0,1).grapheme,"w");
}
