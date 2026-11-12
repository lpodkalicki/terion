/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <gtest/gtest.h>
#include <terion/terion.h>

using namespace terion;

TEST(LogView, BoundedBufferDropsOldestEntries)
{
    LogBuffer buffer(2);
    buffer.append({.timestamp = "1",.message = "first"});
    buffer.append({.timestamp = "2",.message = "second"});
    buffer.append({.timestamp = "3",.message = "third"});
    ASSERT_EQ(buffer.size(),2u);
    EXPECT_EQ(buffer.at(0).message,"second");
    EXPECT_EQ(buffer.at(1).message,"third");
    buffer.clear();
    EXPECT_EQ(buffer.size(),0u);
}

TEST(LogView, FiltersEntriesAndRendersOnlyVisibleRows)
{
    auto buffer = std::make_shared<LogBuffer>(100000);
    for (int index = 0; index < 100000; ++index)
        buffer->append({.timestamp = "10:00",.level = index % 2 ? LogLevel::Error : LogLevel::Info,
            .message = "Message "+std::to_string(index)});
    Reconciler tree;
    tree.update(LogView({.buffer = buffer,.min_level = LogLevel::Error,
        .search = "MESSAGE",.layout = {.width = 36,.height = 4}}));
    ASSERT_NE(tree.root(),nullptr);
    EXPECT_TRUE(tree.root()->children().empty());
    tree.root()->layout({.maximum = {.width = 36,.height = 4}});
    ScreenBuffer screen({.width = 36,.height = 4});
    Painter painter(screen);
    tree.root()->paint(painter);
    EXPECT_EQ(screen.at(0,0).grapheme,"1");
    EXPECT_EQ(tree.root()->virtual_list_offset(),0);
    EXPECT_TRUE(tree.root()->scrollBy(40000));
    tree.root()->paint(painter);
    EXPECT_EQ(tree.root()->virtual_list_offset(),40000);
}

TEST(LogView, SeverityUsesThemeColors)
{
    auto buffer = std::make_shared<LogBuffer>();
    buffer->append({.level = LogLevel::Error,.message = "failure"});
    Reconciler tree;
    tree.update(LogView({.buffer = buffer,.show_timestamp = false,
        .layout = {.width = 30,.height = 2}}));
    tree.root()->layout({.maximum = {.width = 30,.height = 2}});
    ScreenBuffer screen({.width = 30,.height = 2});
    Painter painter(screen);
    const Theme theme = Theme::dark();
    tree.root()->paint(painter,{},{},std::nullopt,&theme);
    EXPECT_EQ(screen.at(0,0).grapheme,"E");
    EXPECT_EQ(screen.at(0,0).style.foreground,theme.palette.error);
}

TEST(LogView, FollowTailTracksNewEntriesButPreservesManualScroll)
{
    auto buffer = std::make_shared<LogBuffer>();
    for (int index = 0; index < 20; ++index)
        buffer->append({.message = "Line " + std::to_string(index)});
    Reconciler tree;
    const auto component = [&] {
        return LogView({.buffer = buffer,.follow_tail = true,
            .layout = {.width = 30,.height = 4}});
    };
    tree.update(component());
    tree.root()->layout({.maximum = {.width = 30,.height = 4}});
    EXPECT_EQ(tree.root()->virtual_list_offset(),16);
    buffer->append({.message = "new"});
    tree.update(component());
    tree.root()->layout({.maximum = {.width = 30,.height = 4}});
    EXPECT_EQ(tree.root()->virtual_list_offset(),17);
    EXPECT_TRUE(tree.root()->scrollBy(-5));
    const int manual_offset = tree.root()->virtual_list_offset();
    buffer->append({.message = "newer"});
    tree.update(component());
    tree.root()->layout({.maximum = {.width = 30,.height = 4}});
    EXPECT_EQ(tree.root()->virtual_list_offset(),manual_offset);
}
