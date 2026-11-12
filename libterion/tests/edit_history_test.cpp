/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */
#include <terion/terion.h>
#include <gtest/gtest.h>

using namespace terion;

namespace {
Event ctrl(char32_t c, bool shift = false)
{
    return {.key = Key::Character,.character = c,.shift = shift,.ctrl = true};
}
Event character(char32_t c) { return {.key = Key::Character,.character = c}; }
} /* End of anonymous namespace */

TEST(EditHistory, GroupsConsecutiveTypingAndRestoresUtf8)
{
    Reconciler reconciler;
    reconciler.update(TextArea({.value = "",.key = "editor"}));
    auto* editor = reconciler.root();
    editor->handleTextInput(character(U'ż'));
    editor->handleTextInput(character(U'ó'));
    editor->handleTextInput(character(U'ł'));
    EXPECT_EQ(editor->text(),"żół");
    EXPECT_TRUE(editor->handleTextInput(ctrl(U'z')));
    EXPECT_EQ(editor->text(),"");
    EXPECT_TRUE(editor->handleTextInput(ctrl(U'y')));
    EXPECT_EQ(editor->text(),"żół");
}

TEST(EditHistory, PasteAndCutEachUndoAsSingleAction)
{
    Reconciler reconciler;
    reconciler.update(TextArea({.value = "first",.key = "editor"}));
    auto* editor = reconciler.root();
    editor->handleTextInput({.key = Key::Paste,.text = "\nsecond"});
    EXPECT_EQ(editor->text(),"first\nsecond");
    editor->handleTextInput(ctrl(U'a'));
    editor->handleTextInput(ctrl(U'x'));
    EXPECT_EQ(editor->text(),"");
    editor->handleTextInput(ctrl(U'z'));
    EXPECT_EQ(editor->text(),"first\nsecond");
    editor->handleTextInput(ctrl(U'z'));
    EXPECT_EQ(editor->text(),"first");
}

TEST(EditHistory, NewEditInvalidatesRedo)
{
    Reconciler reconciler;
    reconciler.update(TextField({.value = "",.key = "field"}));
    auto* editor = reconciler.root();
    editor->handleTextInput(character(U'a'));
    editor->handleTextInput(ctrl(U'z'));
    editor->handleTextInput(character(U'b'));
    editor->handleTextInput(ctrl(U'y'));
    EXPECT_EQ(editor->text(),"b");
}

TEST(EditHistory, ReadOnlyDoesNotModifyHistory)
{
    Reconciler reconciler;
    reconciler.update(TextField({.value = "locked",.read_only = true}));
    auto* editor = reconciler.root();
    editor->handleTextInput(character(U'a'));
    editor->handleTextInput(ctrl(U'z'));
    EXPECT_EQ(editor->text(),"locked");
}

TEST(Clipboard, ScopedWriterPublishesCopiesAndRestoresPreviousSink)
{
    Clipboard::clear();
    std::string sent;
    {
        Clipboard::ScopedWriter writer([&](std::string_view value) { sent = value; });
        Clipboard::set("żółć");
    }
    EXPECT_EQ(sent,"żółć");
    EXPECT_EQ(Clipboard::get(),"żółć");
    Clipboard::clear();
}
