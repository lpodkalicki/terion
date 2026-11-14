/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <terion/terion.h>
#include <terion/text_editing.h>
#include <gtest/gtest.h>

using namespace terion;

TEST(AdvancedTextEditing, WordBoundariesPreserveUtf8)
{
    constexpr std::string_view text = "Hello, zażółć świat!";
    EXPECT_EQ(detail::nextWordBoundary(text,0),5u);
    EXPECT_EQ(detail::nextWordBoundary(text,5),17u);
    EXPECT_EQ(detail::previousWordBoundary(text,text.size()),18u);
    EXPECT_EQ(detail::wordAt(text,9),(std::pair<std::size_t,std::size_t>{7,17}));
}

TEST(AdvancedTextEditing, WordNavigationSelectionAndDeletion)
{
    Clipboard::clear();
    std::string edited;
    Reconciler reconciler;
    reconciler.update(TextField({.value = "hello zażółć",.key = "input",
        .on_change = [&](std::string text) { edited = std::move(text); }}));
    auto* node = reconciler.root();
    ASSERT_TRUE(node->handleTextInput({.key = Key::Left,.shift = true,.alt = true}));
    ASSERT_TRUE(node->handleTextInput({.key = Key::Character,.character = U'c',.ctrl = true}));
    EXPECT_EQ(Clipboard::get(),"zażółć");
    ASSERT_TRUE(node->handleTextInput({.key = Key::Backspace}));
    EXPECT_EQ(edited,"hello ");
    ASSERT_TRUE(node->handleTextInput({.key = Key::Character,.character = U'z',.ctrl = true}));
    EXPECT_EQ(edited,"hello zażółć");
    ASSERT_TRUE(node->handleTextInput({.key = Key::Backspace,.alt = true}));
    EXPECT_EQ(edited,"hello ");
}

TEST(AdvancedTextEditing, DoubleAndTripleClickSelectText)
{
    Clipboard::clear();
    Reconciler reconciler;
    reconciler.update(TextArea({.value = "first word\nsecond line",
        .layout = {.width = 26,.height = 6}}));
    auto* node = reconciler.root();
    node->layout(Constraints{.maximum = {.width = 26,.height = 6}});
    node->selectWordAt({.x = 9,.y = 1});
    ASSERT_TRUE(node->handleTextInput({.key = Key::Character,.character = U'c',.ctrl = true}));
    EXPECT_EQ(Clipboard::get(),"word");
    node->selectLineAt({.x = 2,.y = 2});
    ASSERT_TRUE(node->handleTextInput({.key = Key::Character,.character = U'c',.ctrl = true}));
    EXPECT_EQ(Clipboard::get(),"second line");
}

TEST(AdvancedTextEditing, SoftWrapRendersAndHitTestsVisualRows)
{
    Reconciler reconciler;
    reconciler.update(TextArea({.value = "abcdefghij",.soft_wrap = true,
        .layout = {.width = 6,.height = 6}}));
    auto* node = reconciler.root();
    node->layout(Constraints{.maximum = {.width = 6,.height = 6}});
    ScreenBuffer buffer({.width = 6,.height = 6});
    Painter painter(buffer);
    node->paint(painter);
    EXPECT_EQ(buffer.at(1,1).grapheme,"a");
    EXPECT_EQ(buffer.at(1,2).grapheme,"e");
    EXPECT_EQ(buffer.at(1,3).grapheme,"i");
    node->setTextCursor({.x = 2,.y = 2});
    ASSERT_TRUE(node->handleTextInput({.key = Key::Character,.character = U'X'}));
    EXPECT_EQ(node->text(),"abcdeXfghij");
}

TEST(AdvancedTextEditing, SoftWrapUpDownAndSelection)
{
    Clipboard::clear();
    Reconciler reconciler;
    reconciler.update(TextArea({.value = "abcdefghij",.soft_wrap = true,
        .layout = {.width = 6,.height = 6}}));
    auto* node = reconciler.root();
    node->layout(Constraints{.maximum = {.width = 6,.height = 6}});
    ASSERT_TRUE(node->handleTextInput({.key = Key::Home,.ctrl = true}));
    ASSERT_TRUE(node->handleTextInput({.key = Key::Down}));
    ASSERT_TRUE(node->handleTextInput({.key = Key::Right,.shift = true}));
    ASSERT_TRUE(node->handleTextInput({.key = Key::Character,.character = U'c',.ctrl = true}));
    EXPECT_EQ(Clipboard::get(),"e");
}

TEST(AdvancedTextEditing, InputDecoderHandlesOptionBackspace)
{
    InputDecoder decoder;
    decoder.feed("\033\177\033[1;2D\033[1;3C");
    const auto alt_backspace = decoder.nextEvent();
    ASSERT_TRUE(alt_backspace);
    EXPECT_EQ(alt_backspace->key,Key::Backspace);
    EXPECT_TRUE(alt_backspace->alt);
    const auto shift_left = decoder.nextEvent();
    ASSERT_TRUE(shift_left);
    EXPECT_EQ(shift_left->key,Key::Left);
    EXPECT_TRUE(shift_left->shift);
    const auto alt_right = decoder.nextEvent();
    ASSERT_TRUE(alt_right);
    EXPECT_EQ(alt_right->key,Key::Right);
    EXPECT_TRUE(alt_right->alt);
}

TEST(AdvancedTextEditing, ApplicationDoubleClickSelectsWordAndTripleClickSelectsLine)
{
    Clipboard::clear();
    Application app(std::make_unique<HeadlessTerminalBackend>(Size{30,8}));
    app.render([] { return TextArea({.value = "alpha beta\nsecond line",
        .key = "source",.layout = {.width = 30,.height = 8}}); });
    app.renderOnce();
    const auto click = [&](MouseAction action) {
        return app.dispatchEvent({.key = Key::Mouse,.mouse = {
            .position = {.x = 9,.y = 1},.button = MouseButton::Left,.action = action}});
    };
    ASSERT_TRUE(click(MouseAction::Press));
    ASSERT_TRUE(click(MouseAction::Release));
    ASSERT_TRUE(click(MouseAction::Press));
    ASSERT_TRUE(click(MouseAction::Release));
    ASSERT_TRUE(app.dispatchEvent({.key = Key::Character,.character = U'c',.ctrl = true}));
    EXPECT_EQ(Clipboard::get(),"beta");
    ASSERT_TRUE(click(MouseAction::Press));
    ASSERT_TRUE(click(MouseAction::Release));
    ASSERT_TRUE(app.dispatchEvent({.key = Key::Character,.character = U'c',.ctrl = true}));
    EXPECT_EQ(Clipboard::get(),"alpha beta\n");
}

TEST(AdvancedTextEditing, SoftWrapReflowKeepsTopVisibleText)
{
    Reconciler reconciler;
    reconciler.update(TextArea({.value = "abcdefghijklmnopqrstuvwx",.soft_wrap = true,
        .layout = {.width = 6,.height = 4}}));
    auto* node = reconciler.root();
    node->layout(Constraints{.maximum = {.width = 6,.height = 4}});
    ScreenBuffer buffer({.width = 12,.height = 4});
    Painter painter(buffer);
    node->paint(painter);
    ASSERT_TRUE(node->scrollTextArea(2));
    buffer.clear();
    node->paint(painter);
    EXPECT_EQ(buffer.at(1,1).grapheme,"i");
    node->layout(Constraints{.maximum = {.width = 10,.height = 4}});
    buffer.clear();
    node->paint(painter);
    EXPECT_EQ(buffer.at(1,1).grapheme,"i");
}

TEST(AdvancedTextEditing, NewlineIsASeparateCaretBoundary)
{
    constexpr std::string_view text = "one\ntwo";
    EXPECT_EQ(detail::nextBoundary(text,3),4u);
    EXPECT_EQ(detail::previousBoundary(text,4),3u);
    EXPECT_EQ(detail::wordAt(text,1),(std::pair<std::size_t,std::size_t>{0,3}));
    EXPECT_EQ(detail::wordAt(text,5),(std::pair<std::size_t,std::size_t>{4,7}));
}

TEST(AdvancedTextEditing, SoftWrapKeepsCaretVisibleAtExactWidth)
{
    const auto lines = detail::visualTextLines("abcd",4,true);
    ASSERT_EQ(lines.size(),2u);
    EXPECT_EQ(detail::visualLineIndex(lines,4),1u);
    EXPECT_EQ(lines[1].begin,4u);
    EXPECT_EQ(lines[1].end,4u);
}

TEST(AdvancedTextEditing, ManualScrollDoesNotSnapBackToCursor)
{
    Reconciler reconciler;
    reconciler.update(TextArea({.value = "abcdefghijklmnopqrstuvwx",.soft_wrap = true,
        .layout = {.width = 6,.height = 4}}));
    auto* node = reconciler.root();
    node->layout(Constraints{.maximum = {.width = 6,.height = 4}});
    ScreenBuffer buffer({.width = 6,.height = 4});
    Painter painter(buffer);
    const auto focused = std::to_string(node->id());
    node->paint(painter,focused);
    ASSERT_TRUE(node->scrollTextArea(-2));
    buffer.clear();
    node->paint(painter,focused);
    EXPECT_EQ(buffer.at(1,1).grapheme,"m");
}

TEST(AdvancedTextEditing, OptionBAndFMoveByWord)
{
    Clipboard::clear();
    Reconciler reconciler;
    reconciler.update(TextField({.value = "one two"}));
    auto* node = reconciler.root();
    ASSERT_TRUE(node->handleTextInput({.key = Key::Character,.character = U'b',.alt = true}));
    ASSERT_TRUE(node->handleTextInput({.key = Key::Left,.shift = true}));
    ASSERT_TRUE(node->handleTextInput({.key = Key::Character,.character = U'c',.ctrl = true}));
    EXPECT_EQ(Clipboard::get()," ");
    ASSERT_TRUE(node->handleTextInput({.key = Key::Character,.character = U'f',.alt = true}));
}

TEST(AdvancedTextEditing, ShiftClickExtendsExistingSelection)
{
    Clipboard::clear();
    Application app(std::make_unique<HeadlessTerminalBackend>(Size{24,5}));
    app.render([] { return TextField({.value = "hello world",
        .key = "source",.layout = {.width = 24}}); });
    app.renderOnce();
    const auto press = [&](int x, bool shift) {
        return app.dispatchEvent({.key = Key::Mouse,.mouse = {
            .position = {.x = x,.y = 1},.button = MouseButton::Left,
            .action = MouseAction::Press,.shift = shift}});
    };
    const auto release = [&] {
        return app.dispatchEvent({.key = Key::Mouse,.mouse = {
            .position = {.x = 6,.y = 1},.button = MouseButton::Left,
            .action = MouseAction::Release}});
    };
    ASSERT_TRUE(press(2,false));
    ASSERT_TRUE(release());
    ASSERT_TRUE(press(7,true));
    ASSERT_TRUE(release());
    ASSERT_TRUE(app.dispatchEvent({.key = Key::Character,.character = U'c',.ctrl = true}));
    EXPECT_EQ(Clipboard::get(),"hello");
}
