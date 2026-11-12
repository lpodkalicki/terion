/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <terion/terion.h>
#include <gtest/gtest.h>

using namespace terion;

TEST(CommandPalette, ProducesModalDialogAndSearchableAutocomplete)
{
    const CommandPalette palette({.open = true,.commands = {
        {.id = "open",.title = "Open",.description = "File",.shortcut = "Ctrl+O"},
        {.id = "disabled",.title = "Restricted",.disabled = true}},
        .key = "palette"});
    EXPECT_EQ(palette.props().type,Component::Type::Dialog);
    EXPECT_EQ(palette.props().key,"palette");
    EXPECT_TRUE(palette.props().dialog_open);
    EXPECT_FALSE(palette.props().dialog_show_actions);
    ASSERT_EQ(palette.props().children.size(),2U);
    const auto& search = palette.props().children[1].props();
    EXPECT_EQ(search.type,Component::Type::Autocomplete);
    ASSERT_EQ(search.select_options.size(),2U);
    EXPECT_EQ(search.select_options[0].value,"open");
    EXPECT_NE(search.select_options[0].label.find("Ctrl+O"),std::string::npos);
    EXPECT_TRUE(search.select_options[1].disabled);
}

TEST(CommandPalette, DispatchesCommandAndCloses)
{
    std::string executed;
    int closed = 0;
    const CommandPalette palette({.open = true,.commands = {{.id = "reload",.title = "Reload"}},
        .on_execute = [&](std::string id) { executed = std::move(id); },
        .on_close = [&] { ++closed; }});
    const auto& search = palette.props().children[1].props();
    ASSERT_TRUE(static_cast<bool>(search.on_change));
    search.on_change("reload");
    EXPECT_EQ(executed,"reload");
    EXPECT_EQ(closed,1);
}

TEST(CommandPalette, InputUpdatesControlledQuery)
{
    std::string query;
    const CommandPalette palette({.open = true,.query = "re",
        .on_query_change = [&](std::string value) { query = std::move(value); }});
    const auto& search = palette.props().children[1].props();
    EXPECT_EQ(search.autocomplete_query,"re");
    ASSERT_TRUE(static_cast<bool>(search.autocomplete_on_input_change));
    search.autocomplete_on_input_change("restart");
    EXPECT_EQ(query,"restart");
}
