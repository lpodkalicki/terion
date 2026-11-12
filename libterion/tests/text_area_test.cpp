/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <terion/terion.h>
#include <gtest/gtest.h>

using namespace terion;

TEST(TextArea, MultilineEditAndUtf8Navigation)
{
    std::string output;
    Reconciler reconciler;
    reconciler.update(TextArea({.value = "Ł\n東京",.key = "editor",
        .on_change = [&](std::string value) { output = std::move(value); }}));
    auto* node = reconciler.root();
    node->layout(Constraints{.maximum = {.width = 24,.height = 8}});
    EXPECT_TRUE(node->handleTextInput(Event{.key = Key::Enter}));
    EXPECT_EQ(output,"Ł\n東京\n");
    EXPECT_TRUE(node->handleTextInput(Event{.key = Key::Up}));
    EXPECT_TRUE(node->handleTextInput(Event{.key = Key::End}));
    EXPECT_TRUE(node->handleTextInput(Event{.key = Key::Character,.character = U'!'}));
    EXPECT_EQ(output,"Ł\n東京!\n");
    EXPECT_TRUE(node->handleTextInput(Event{.key = Key::Paste,.text = "\nabc\r\nxyz"}));
    EXPECT_EQ(output,"Ł\n東京!\nabc\nxyz\n");
}

TEST(TextArea, ReadOnlyAndSelection)
{
    std::string output;
    Reconciler reconciler;
    reconciler.update(TextArea({.value = "abc\ndef",.key = "editor",
        .on_change = [&](std::string value) { output = std::move(value); }}));
    auto* node = reconciler.root();
    EXPECT_TRUE(node->handleTextInput(Event{.key = Key::Character,.character = U'a',.ctrl = true}));
    EXPECT_TRUE(node->handleTextInput(Event{.key = Key::Character,.character = U'X'}));
    EXPECT_EQ(output,"X");
    reconciler.update(TextArea({.value = output,.key = "editor",.read_only = true}));
    EXPECT_TRUE(reconciler.root()->handleTextInput(Event{.key = Key::Enter}));
    EXPECT_EQ(reconciler.root()->text(),"X");
}

TEST(TextArea, PaintsMultipleRows)
{
    Reconciler reconciler;
    reconciler.update(TextArea({.value = "one\ntwo",.layout = {.width = 12,.height = 5}}));
    auto* node = reconciler.root();
    node->layout(Constraints{.maximum = {.width = 12,.height = 5}});
    ScreenBuffer buffer({.width = 12,.height = 5});
    Painter painter(buffer);
    node->paint(painter,{}, {},std::nullopt,nullptr);
    EXPECT_EQ(buffer.at(1,1).grapheme,"o");
    EXPECT_EQ(buffer.at(1,2).grapheme,"t");
}
