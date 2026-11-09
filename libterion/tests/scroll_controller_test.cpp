/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <terion/scroll_controller.h>
#include <gtest/gtest.h>

using namespace terion;

TEST(ScrollController, ClampsOffsetsAndHandlesEmptyContent)
{
    const ScrollController empty(2,5,10);
    EXPECT_EQ(empty.offset(),0);
    EXPECT_EQ(empty.maximum(),0);
    EXPECT_FALSE(empty.scrollable());
    EXPECT_EQ(empty.thumb({.x = 4,.y = 2,.width = 1,.height = 5}).height,0);

    const ScrollController scroll(12,4,20);
    EXPECT_EQ(scroll.maximum(),8);
    EXPECT_EQ(scroll.offset(),8);
    EXPECT_EQ(scroll.scrollBy(-100),0);
    EXPECT_EQ(scroll.scrollBy(100),8);
    EXPECT_EQ(scroll.scrollBy(-3),5);
}

TEST(ScrollController, RevealsFocusedRowWithoutOverscrolling)
{
    const ScrollController scroll(20,5,4);
    EXPECT_EQ(scroll.reveal(6),4);
    EXPECT_EQ(scroll.reveal(2),2);
    EXPECT_EQ(scroll.reveal(9),5);
    EXPECT_EQ(scroll.reveal(18,2),15);
    EXPECT_EQ(ScrollController(20,5,99).reveal(19),15);
    EXPECT_EQ(ScrollController(20,0,5).reveal(19),5);
}

TEST(ScrollController, ThumbAndDragUseIdenticalGeometry)
{
    const Rect track{.x = 7,.y = 3,.width = 1,.height = 4};
    const ScrollController scroll(12,4,4);
    const Rect thumb = scroll.thumb(track);
    EXPECT_EQ(thumb.x,7);
    EXPECT_EQ(thumb.y,4);
    EXPECT_EQ(thumb.height,1);
    EXPECT_EQ(scroll.offsetAt(track,thumb.y,0),3);
    EXPECT_EQ(scroll.offsetAt(track,track.y,0),0);
    EXPECT_EQ(scroll.offsetAt(track,track.y+track.height,0),8);
}

TEST(ScrollController, HandlesSingleCellTrack)
{
    const ScrollController scroll(100,1,42);
    const Rect track{.x = 0,.y = 0,.width = 1,.height = 1};
    EXPECT_EQ(scroll.thumb(track).height,1);
    EXPECT_EQ(scroll.offsetAt(track,1,0),0);
}
