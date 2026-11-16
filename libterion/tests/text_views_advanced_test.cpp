/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <gtest/gtest.h>
#include <terion/terion.h>

using namespace terion;

namespace {

std::string joined(const StyledText& line)
{
    std::string result;
    for (const auto& span : line) result += span.text;
    return result;
}

} /* End of namespace */

TEST(MarkdownViewAdvanced, OutlineSupportsAtxAndSetextWithoutReadingCode)
{
    const auto headings = MarkdownView::outline("# Root\nText\n===\n````cpp\n# fake\n```\n````\n## Last");
    ASSERT_EQ(headings.size(),3u);
    EXPECT_EQ(headings[0].source_line,0u);
    EXPECT_EQ(headings[1].level,1);
    EXPECT_EQ(headings[1].source_line,1u);
    EXPECT_EQ(headings[2].source_line,7u);
    EXPECT_EQ(headings[2].text,"Last");
}

TEST(MarkdownViewAdvanced, LongFenceRequiresMatchingClosingDelimiter)
{
    const auto rendered = MarkdownView::render("````cpp title=test\nint a;\n```\nint b;\n````");
    ASSERT_EQ(rendered.size(),3u);
    EXPECT_EQ(rendered[0][1].text,"int");
    EXPECT_EQ(rendered[0][1].color,ColorVariant::Primary);
    EXPECT_NE(joined(rendered[1]).find("```"),std::string::npos);
    EXPECT_NE(joined(rendered[2]).find("int b"),std::string::npos);
}

TEST(MarkdownViewAdvanced, AlignedGfmTableHandlesEscapedPipes)
{
    const auto rendered = MarkdownView::render(
        "| Left | Center | Right |\n| :--- | :---: | ---: |\n| A\\|B | x | 7 |");
    ASSERT_EQ(rendered.size(),3u);
    const auto cells = joined(rendered[2]);
    EXPECT_NE(cells.find("A|B"),std::string::npos);
    EXPECT_NE(cells.find(" x "),std::string::npos);
    EXPECT_NE(cells.find("    7"),std::string::npos);
    EXPECT_EQ(detail::textCells(joined(rendered[0])),detail::textCells(cells));
}

TEST(MarkdownViewAdvanced, RendersSetextAndAutolinks)
{
    const auto rendered = MarkdownView::render("Heading\n===\n<https://example.org>");
    ASSERT_EQ(rendered.size(),2u);
    EXPECT_EQ(rendered[0][1].text,"Heading");
    EXPECT_TRUE(rendered[0][1].bold);
    ASSERT_EQ(rendered[1].size(),1u);
    EXPECT_EQ(rendered[1][0].text,"https://example.org");
    EXPECT_TRUE(rendered[1][0].underline);
}

TEST(MarkdownViewAdvanced, ClickReportsOriginalLineAfterWrapping)
{
    std::size_t clicked = 99;
    Reconciler view;
    view.update(MarkdownView({.markdown="first really long line with words\nsecond line",
        .layout={.width=13,.height=5},
        .on_source_line_click=[&](std::size_t line) { clicked=line; }}));
    view.root()->layout({.maximum={.width=13,.height=5}});
    EXPECT_TRUE(view.root()->handleVirtualListClick({0,1}));
    EXPECT_EQ(clicked,0u);
    EXPECT_TRUE(view.root()->handleVirtualListClick({0,3}));
    EXPECT_EQ(clicked,1u);
}

TEST(DiffViewAdvanced, HunkIndexAndCopyText)
{
    constexpr auto diff = "diff --git a/one b/one\n--- a/one\n+++ b/one\n@@ -3,1 +3,1 @@\n-old\n+new\n@@ -9 +9 @@\n-last\n+next\n";
    const auto locations = DiffView::hunks(diff);
    ASSERT_EQ(locations.size(),2u);
    EXPECT_EQ(locations[0].source_line,3u);
    EXPECT_EQ(locations[0].old_start,3);
    EXPECT_EQ(locations[1].new_start,9);
    EXPECT_EQ(DiffView::hunkText(diff,0),"@@ -3,1 +3,1 @@\n-old\n+new\n");
    EXPECT_EQ(DiffView::hunkText(diff,1),"@@ -9 +9 @@\n-last\n+next\n");
    EXPECT_TRUE(DiffView::hunkText(diff,2).empty());
}

TEST(DiffViewAdvanced, IntralineEmphasizesOnlyChangedGraphemes)
{
    const auto rendered = DiffView::render(
        "@@ -1 +1 @@\n-ab👨‍👩‍👧old\n+ab👨‍👩‍👧new",
        DiffViewMode::Unified,false,0,true);
    ASSERT_EQ(rendered.size(),3u);
    bool old_changed = false;
    bool new_changed = false;
    for (std::size_t r=1; r<rendered.size(); ++r)
        for (const auto& span : rendered[r])
        {
            EXPECT_TRUE(isValidUtf8(span.text));
            if (span.text == "old" && span.underline) old_changed = true;
            if (span.text == "new" && span.underline) new_changed = true;
            if (span.text.find("👨‍👩‍👧") != std::string::npos)
            {
                EXPECT_FALSE(span.underline);
            }
        }
    EXPECT_TRUE(old_changed);
    EXPECT_TRUE(new_changed);
}

TEST(DiffViewAdvanced, SideBySidePairsUnequalChangeBlocksAndPreservesColumns)
{
    const auto rendered = DiffView::render(
        "@@ -1,3 +1,2 @@\n-a\n-b\n+z\n context",
        DiffViewMode::SideBySide,true,52,true);
    ASSERT_EQ(rendered.size(),4u);
    for (const auto& row : rendered)
    {
        EXPECT_EQ(detail::textCells(joined(row)),52);
        for (const auto& span : row) EXPECT_TRUE(isValidUtf8(span.text));
    }
    EXPECT_NE(joined(rendered[1]).find("a"),std::string::npos);
    EXPECT_NE(joined(rendered[1]).find("z"),std::string::npos);
    EXPECT_NE(joined(rendered[2]).find("b"),std::string::npos);
    EXPECT_NE(joined(rendered[3]).find("context"),std::string::npos);
}

TEST(DiffViewAdvanced, ModeFallsBackToUnifiedWhenTooNarrow)
{
    const auto rendered = DiffView::render("@@ -1 +1 @@\n-a\n+b",
        DiffViewMode::SideBySide,false,12,true);
    ASSERT_EQ(rendered.size(),3u);
    EXPECT_NE(joined(rendered[1]).find("-a"),std::string::npos);
}

TEST(DiffViewAdvanced, CanRevealActiveHunkWithoutRecreatingView)
{
    const std::string diff = "@@ -1 +1 @@\n-a\n+b\n context\n@@ -9 +9 @@\n-c\n+d";
    Reconciler view;
    view.update(DiffView({.diff=diff,.key="diff-view",.layout={.width=30,.height=2}}));
    view.root()->layout({.maximum={.width=30,.height=2}});
    view.update(DiffView({.diff=diff,.key="diff-view",.layout={.width=30,.height=2},
        .active_hunk=1}));
    view.root()->layout({.maximum={.width=30,.height=2}});
    EXPECT_TRUE(view.root()->virtual_list_offset() > 0);
}

TEST(DiffViewAdvanced, CopyPreservesCrlfAndMissingFinalNewline)
{
    const auto lines = DiffView::hunkText("@@ -1 +1 @@\r\n-a\r\n+b",0);
    EXPECT_EQ(lines,"@@ -1 +1 @@\r\n-a\r\n+b");
}

TEST(DiffViewAdvanced, CopyStopsAtNextFileHeader)
{
    const auto lines = DiffView::hunkText("@@ -1 +1 @@\n-a\n+b\ndiff --git a/b b/b\n@@ -1 +1 @@\n-c\n+d\n",0);
    EXPECT_EQ(lines,"@@ -1 +1 @@\n-a\n+b\n");
}

TEST(DiffViewAdvanced, SideBySideReflowsOnResize)
{
    Reconciler view;
    view.update(DiffView({.diff="@@ -1 +1 @@\n-old name 👨‍👩‍👧\n+new name 👨‍👩‍👧",
        .layout={.height=3},.mode=DiffViewMode::SideBySide,.highlight_intraline=true}));
    view.root()->layout({.maximum={.width=70,.height=3}});
    ScreenBuffer wide({.width=70,.height=3});
    Painter wide_painter(wide);
    view.root()->paint(wide_painter);
    view.root()->layout({.maximum={.width=35,.height=3}});
    ScreenBuffer narrow({.width=35,.height=3});
    Painter narrow_painter(narrow);
    view.root()->paint(narrow_painter);
    EXPECT_EQ(narrow.at(0,0).grapheme,"@");
}

TEST(DiffViewAdvanced, UnicodeSideBySideSeparatorRemainsAtFixedColumn)
{
    const auto rendered = DiffView::render(
        "@@ -20,3 +20,3 @@\n"
        " void onChange() {\n"
        "-    setText(\"Polska 🇵🇱\");\n"
        "+    setText(\"Polska 🇵🇱 · UTF-8 👨‍👩‍👧\");\n"
        " }",
        DiffViewMode::SideBySide,true,72,true);
    ASSERT_EQ(rendered.size(),4u);
    for (std::size_t y = 1; y < rendered.size(); ++y)
    {
        int x = 0;
        int guide = -1;
        for (const auto& span : rendered[y])
            for (std::size_t pos = 0; pos < span.text.size();)
            {
                const auto glyph = nextGrapheme(span.text,pos);
                ASSERT_GT(glyph.bytes,0u);
                if (span.text.substr(pos,glyph.bytes) == "│") guide = x;
                x += glyph.columns;
                pos += glyph.bytes;
            }
        EXPECT_EQ(x,72);
        EXPECT_EQ(guide,35); // (72 - 3) / 2 + one separator pad
    }
}
