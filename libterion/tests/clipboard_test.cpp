/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <terion/terion.h>
#include <gtest/gtest.h>

using namespace terion;

namespace {
Event ctrl(char32_t c) { return {.key = Key::Character,.character = c,.ctrl = true}; }
} /* End of anonymous namespace */

TEST(Clipboard, CopiesAndPastesBetweenFields)
{
    Clipboard::clear();
    Reconciler reconciler;
    reconciler.update(TextField({.value = "Zażółć",.key = "source"}));
    auto* source = reconciler.root();
    EXPECT_TRUE(source->handleTextInput(ctrl(U'a')));
    EXPECT_TRUE(source->handleTextInput(ctrl(U'c')));
    EXPECT_EQ(Clipboard::get(),"Zażółć");
    reconciler.update(TextArea({.value = "",.key = "target"}));
    auto* target = reconciler.root();
    EXPECT_TRUE(target->handleTextInput(ctrl(U'v')));
    EXPECT_EQ(target->text(),"Zażółć");
}

TEST(Clipboard, CutAndPasteReplaceSelection)
{
    Clipboard::clear();
    Reconciler reconciler;
    reconciler.update(TextArea({.value = "abc\ndef",.key = "editor"}));
    auto* node = reconciler.root();
    EXPECT_TRUE(node->handleTextInput(ctrl(U'a')));
    EXPECT_TRUE(node->handleTextInput(ctrl(U'x')));
    EXPECT_EQ(node->text(),"");
    EXPECT_EQ(Clipboard::get(),"abc\ndef");
    EXPECT_TRUE(node->handleTextInput(ctrl(U'v')));
    EXPECT_EQ(node->text(),"abc\ndef");
}

TEST(Clipboard, PasswordCannotBeCopiedOrCut)
{
    Clipboard::set("safe");
    Reconciler reconciler;
    reconciler.update(PasswordField({.value = "secret",.key = "password"}));
    auto* node = reconciler.root();
    EXPECT_TRUE(node->handleTextInput(ctrl(U'a')));
    EXPECT_TRUE(node->handleTextInput(ctrl(U'c')));
    EXPECT_TRUE(node->handleTextInput(ctrl(U'x')));
    EXPECT_EQ(node->text(),"secret");
    EXPECT_EQ(Clipboard::get(),"safe");
}

TEST(Clipboard, ReadOnlyAllowsCopyButNotCutOrPaste)
{
    Clipboard::clear();
    Reconciler reconciler;
    reconciler.update(TextArea({.value = "read only",.read_only = true}));
    auto* node = reconciler.root();
    EXPECT_TRUE(node->handleTextInput(ctrl(U'a')));
    EXPECT_TRUE(node->handleTextInput(ctrl(U'c')));
    EXPECT_EQ(Clipboard::get(),"read only");
    EXPECT_TRUE(node->handleTextInput(ctrl(U'x')));
    EXPECT_EQ(node->text(),"read only");
    EXPECT_TRUE(node->handleTextInput(ctrl(U'v')));
    EXPECT_EQ(node->text(),"read only");
}

TEST(Clipboard, NumberFieldRejectsInvalidClipboardText)
{
    Clipboard::set("not a number");
    Reconciler reconciler;
    reconciler.update(NumberField({.value = std::string("123")}));
    auto* node = reconciler.root();
    EXPECT_TRUE(node->handleTextInput(ctrl(U'v')));
    EXPECT_EQ(node->text(),"123");
}

TEST(Clipboard, BracketedPasteRetainsNewlinesForTextArea)
{
    InputDecoder decoder;
    decoder.feed("\033[200~first\nsecond\033[201~");
    const auto event = decoder.nextEvent();
    ASSERT_TRUE(event.has_value());
    EXPECT_EQ(event->key,Key::Paste);
    EXPECT_EQ(event->text,"first\nsecond");
}
