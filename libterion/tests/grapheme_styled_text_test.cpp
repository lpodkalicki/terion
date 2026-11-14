/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <terion/terion.h>
#include <terion/text_editing.h>
#include <terion/components/text_view_detail.h>
#include <gtest/gtest.h>

using namespace terion;

TEST(Graphemes, CombiningEmojiFlagsAndSequencesAreSingleClusters)
{
    constexpr std::string_view acute = "e\u0301";
    constexpr std::string_view family = "👨‍👩‍👧";
    constexpr std::string_view flag = "🇵🇱";
    constexpr std::string_view skin = "👍🏽";
    constexpr std::string_view heart = "❤️";
    constexpr std::string_view keycap = "1️⃣";
    for (const auto text : {acute,family,flag,skin,heart,keycap})
    {
        const auto cluster = nextGrapheme(text,0);
        EXPECT_EQ(cluster.bytes,text.size());
        EXPECT_EQ(nextGraphemeBoundary(text,0),text.size());
        EXPECT_EQ(previousGraphemeBoundary(text,text.size()),0u);
    }
    EXPECT_EQ(graphemeWidth(acute),1);
    for (const auto text : {family,flag,skin,heart,keycap}) EXPECT_EQ(graphemeWidth(text),2);
    EXPECT_EQ(displayWidth("A👨‍👩‍👧e\u0301🇵🇱Z"),7);
    EXPECT_EQ(nextGrapheme("\r\n",0).bytes,2u);
    EXPECT_EQ(nextGrapheme("\nA",0).bytes,1u);
}

TEST(Graphemes, PainterNeverSplitsEmojiAcrossCells)
{
    ScreenBuffer screen({.width = 9,.height = 3});
    Painter painter(screen);
    painter.text(0,0,"A👨‍👩‍👧Z");
    EXPECT_EQ(screen.at(0,0).grapheme,"A");
    EXPECT_EQ(screen.at(1,0).grapheme,"👨‍👩‍👧");
    EXPECT_EQ(screen.at(1,0).width,2);
    EXPECT_EQ(screen.at(2,0).width,0);
    EXPECT_EQ(screen.at(3,0).grapheme,"Z");
    painter.put(2,0,"B");
    EXPECT_EQ(screen.at(1,0).grapheme," ");
    EXPECT_EQ(screen.at(2,0).grapheme,"B");
    painter.text(8,1,"🇵🇱");
    EXPECT_EQ(screen.at(8,1).grapheme," ");
    painter.text(7,1,"🇵🇱");
    EXPECT_EQ(screen.at(7,1).grapheme,"🇵🇱");
    EXPECT_EQ(screen.at(8,1).width,0);
}

TEST(Graphemes, EditorMovesAndDeletesWholeGraphemes)
{
    Clipboard::clear();
    Reconciler reconciler;
    std::string value;
    reconciler.update(TextField({.value = "A👨‍👩‍👧B",.key = "emoji",
        .on_change = [&](std::string edited) { value = std::move(edited); }}));
    auto* node = reconciler.root();
    ASSERT_TRUE(node->handleTextInput({.key = Key::Left}));
    ASSERT_TRUE(node->handleTextInput({.key = Key::Backspace}));
    EXPECT_EQ(value,"AB");
    constexpr std::string_view word = "a🇵🇱b";
    const auto rows = detail::visualTextLines(word,3,true);
    ASSERT_EQ(rows.size(),2u);
    EXPECT_EQ(rows[0].end,std::string_view("a🇵🇱").size());
    EXPECT_EQ(detail::visualColumn(word,rows[0],rows[0].end),3);
    EXPECT_EQ(detail::visualOffset(word,rows[0],2),1u);
}

TEST(StyledText, WrapPreservesEveryAttributeAndWholeGraphemes)
{
    StyledSpan formatted{.text = "👨‍👩‍👧 hello",.color = ColorVariant::Success,
        .bold = true,.italic = true,.underline = true,.strikethrough = true,
        .dim = true,.background = ColorVariant::Warning};
    const auto rows = detail::wrapStyled({formatted},4);
    ASSERT_GE(rows.size(),2u);
    ASSERT_FALSE(rows[0].empty());
    EXPECT_EQ(rows[0][0].text.substr(0,std::string("👨‍👩‍👧").size()),"👨‍👩‍👧");
    for (const auto& row : rows)
    {
        for (const auto& span : row)
        {
            EXPECT_TRUE(span.bold);
            EXPECT_TRUE(span.italic);
            EXPECT_TRUE(span.underline);
            EXPECT_TRUE(span.strikethrough);
            EXPECT_TRUE(span.dim);
            ASSERT_TRUE(span.background.has_value());
            EXPECT_EQ(*span.background,ColorVariant::Warning);
        }
    }
}

TEST(StyledText, VirtualListAppliesBackgroundAndAnsiAttributes)
{
    Reconciler reconciler;
    reconciler.update(VirtualList({.item_count = 1,
        .item_styled = [](int) -> StyledText {
            return {{.text = "OK",.color = ColorVariant::Success,.bold = true,
                .italic = true,.underline = true,.strikethrough = true,
                .dim = true,.background = ColorVariant::Warning}};
        },.layout = {.width = 10,.height = 2},.show_scrollbar = false}));
    auto* node = reconciler.root();
    node->layout(Constraints{.maximum = {.width = 10,.height = 2}});
    ScreenBuffer screen({.width = 10,.height = 2});
    Painter painter(screen);
    node->paint(painter);
    const auto style = screen.at(0,0).style;
    EXPECT_TRUE(style.bold);
    EXPECT_TRUE(style.italic);
    EXPECT_TRUE(style.underline);
    EXPECT_TRUE(style.strikethrough);
    EXPECT_TRUE(style.dim);
    EXPECT_EQ(style.background,Theme::dark().palette.resolve(ColorVariant::Warning));
    HeadlessTerminalBackend backend({10,2});
    DifferentialRenderer renderer;
    renderer.render(screen,backend);
    EXPECT_NE(backend.output().find(";3;"),std::string::npos);
    EXPECT_NE(backend.output().find(";9;"),std::string::npos);
    EXPECT_NE(backend.output().find(";2;"),std::string::npos);
}

TEST(StyledText, MarkdownInlineFormattingUsesRealAnsiAttributes)
{
    const auto lines = MarkdownView::render("*italic* ~~deleted~~ [link](https://example.org)");
    ASSERT_EQ(lines.size(),1u);
    bool italic = false, strike = false, underline = false;
    for (const auto& span : lines[0])
    {
        if (span.text == "italic") italic = span.italic;
        if (span.text == "deleted") strike = span.strikethrough;
        if (span.text == "link") underline = span.underline;
    }
    EXPECT_TRUE(italic);
    EXPECT_TRUE(strike);
    EXPECT_TRUE(underline);
}

TEST(Graphemes, DifferentialRendererReanchorsAfterCompositeEmojiAndCombiningMarks)
{
    ScreenBuffer buffer({.width = 12,.height = 2});
    Painter painter(buffer);
    painter.text(0,0,"A👨‍👩‍👧Z");
    painter.text(0,1,"e\u0301X");
    HeadlessTerminalBackend backend({12,2});
    DifferentialRenderer renderer;
    renderer.render(buffer,backend);
    const std::string output = backend.output();
    // The terminal may advance a different number of columns for composite
    // emoji. Never rely on its implicit cursor position after a grapheme.
    EXPECT_NE(output.find("👨‍👩‍👧\033[1;4H"),std::string::npos);
    EXPECT_NE(output.find("e\u0301\033[2;2H"),std::string::npos);
}

TEST(Graphemes, DifferentialRendererErasesWideGlyphRemnantsOnDeletion)
{
    ScreenBuffer before({.width = 12,.height = 1});
    Painter before_painter(before);
    before_painter.text(0,0,"A👨‍👩‍👧Z");
    HeadlessTerminalBackend backend({12,1});
    DifferentialRenderer renderer;
    renderer.render(before,backend);
    backend.clearOutput();

    ScreenBuffer after({.width = 12,.height = 1});
    Painter after_painter(after);
    after_painter.text(0,0,"AZ");
    renderer.render(after,backend);
    EXPECT_NE(backend.output().find("\033[1;1H\033[2K"),std::string::npos);
    EXPECT_NE(backend.output().find("AZ"),std::string::npos);

    backend.clearOutput();
    renderer.render(after,backend);
    EXPECT_TRUE(backend.output().empty());
}

TEST(Graphemes, TextAreaBackspaceDeletesCompleteEmojiSequences)
{
    Reconciler reconciler;
    std::string edited;
    reconciler.update(TextArea({.value = "A👨‍👩‍👧🇵🇱👍🏽Z",.soft_wrap = true,
        .layout = {.width = 20,.height = 5},
        .on_change = [&](std::string text) { edited = std::move(text); }}));
    auto* node = reconciler.root();
    ASSERT_NE(node,nullptr);
    node->layout(Constraints{.maximum = {.width = 20,.height = 5}});
    ASSERT_TRUE(node->handleTextInput({.key = Key::Left}));
    for (const auto expected : {"A👨‍👩‍👧🇵🇱Z","A👨‍👩‍👧Z","AZ"})
    {
        ASSERT_TRUE(node->handleTextInput({.key = Key::Backspace}));
        EXPECT_EQ(edited,expected);
    }
    ScreenBuffer screen({.width = 20,.height = 5});
    Painter painter(screen);
    node->paint(painter,std::to_string(node->id()));
    EXPECT_EQ(screen.at(1,1).grapheme,"A");
    EXPECT_EQ(screen.at(2,1).grapheme,"Z");
}
