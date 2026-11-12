/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <terion/terion.h>
#include <gtest/gtest.h>

using namespace terion;

TEST(Autocomplete, FiltersInputAndSelectsByKeyboard)
{
    std::string chosen;
    std::string typed;
    Reconciler reconciler;
    reconciler.update(Autocomplete({.options = {
        {.label = "Warszawa",.value = "waw"},
        {.label = "Lublin",.value = "lub"},
        {.label = "London",.value = "lon",.disabled = true}},
        .layout = {.width = 22},
        .on_change = [&](std::string value) { chosen = std::move(value); },
        .on_input_change = [&](std::string value) { typed = std::move(value); }}));
    auto* node = reconciler.root();
    const Size screen {.width = 40,.height = 14};
    node->layout(Constraints{.maximum = screen});
    EXPECT_TRUE(node->handleSelectKey(Event{.key = Key::Character,.character = U'l'},screen));
    EXPECT_TRUE(node->handleSelectKey(Event{.key = Key::Character,.character = U'u'},screen));
    EXPECT_EQ(typed,"lu");
    ASSERT_EQ(node->autocompleteOptions().size(),1U);
    EXPECT_EQ(node->autocompleteOptions()[0].value,"lub");
    EXPECT_TRUE(node->handleSelectKey(Event{.key = Key::Enter},screen));
    EXPECT_EQ(chosen,"lub");
    EXPECT_FALSE(node->selectOpen());
}

TEST(Autocomplete, EmptyResultsAndPasteAreSafe)
{
    Reconciler reconciler;
    reconciler.update(Autocomplete({.options = {{.label = "Gdańsk",.value = "gdn"}},
        .layout = {.width = 22},.on_change = [](std::string) {}}));
    auto* node = reconciler.root();
    const Size screen {.width = 40,.height = 14};
    node->layout(Constraints{.maximum = screen});
    EXPECT_TRUE(node->handleSelectKey(Event{.key = Key::Paste,.text = "NO MATCH"},screen));
    EXPECT_TRUE(node->selectOpen());
    EXPECT_TRUE(node->autocompleteOptions().empty());
    EXPECT_TRUE(node->handleSelectKey(Event{.key = Key::Down},screen));
    EXPECT_TRUE(node->handleSelectKey(Event{.key = Key::Enter},screen));
    EXPECT_TRUE(node->handleSelectKey(Event{.key = Key::Escape},screen));
    EXPECT_FALSE(node->selectOpen());
    EXPECT_TRUE(node->handleSelectKey(Event{.key = Key::Backspace},screen));
    EXPECT_EQ(node->autocompleteQuery(),"NO MATC");
}

TEST(Autocomplete, RetainsQueryDuringControlledRerender)
{
    Reconciler reconciler;
    const auto make_component = [] {
        return Autocomplete({.options = {{.label = "Lublin",.value = "lub"}},
            .query = "",.layout = {.width = 24}});
    };
    reconciler.update(make_component());
    auto* node = reconciler.root();
    const Size screen {.width = 40,.height = 14};
    node->layout(Constraints{.maximum = screen});
    EXPECT_TRUE(node->handleSelectKey(Event{.key = Key::Character,.character = U'l'},screen));
    reconciler.update(make_component());
    EXPECT_EQ(reconciler.root()->autocompleteQuery(),"l");
}

TEST(Autocomplete, MouseChoosesFilteredOptionAndDisabledItemsDoNotSelect)
{
    std::string chosen;
    Reconciler reconciler;
    reconciler.update(Autocomplete({.options = {
        {.label = "Alpha",.value = "a",.disabled = true},
        {.label = "Alpine",.value = "b"},
        {.label = "Beta",.value = "c"}},
        .layout = {.width = 20},
        .on_change = [&](std::string value) { chosen = std::move(value); }}));
    auto* node = reconciler.root();
    const Size screen {.width = 40,.height = 12};
    node->layout(Constraints{.maximum = screen});
    EXPECT_TRUE(node->handleSelectKey(Event{.key = Key::Paste,.text = "Al"},screen));
    EXPECT_EQ(node->autocompleteOptions().size(),2U);
    const Rect popup = node->selectPopupRect(screen);
    EXPECT_TRUE(node->handleSelectMouse(MouseEvent{.position = {.x = popup.x+1,.y = popup.y},
        .button = MouseButton::Left,.action = MouseAction::Press},screen));
    EXPECT_EQ(chosen,"");
    EXPECT_TRUE(node->handleSelectMouse(MouseEvent{.position = {.x = popup.x+1,.y = popup.y+1},
        .button = MouseButton::Left,.action = MouseAction::Press},screen));
    EXPECT_EQ(chosen,"b");
    EXPECT_FALSE(node->selectOpen());
}
