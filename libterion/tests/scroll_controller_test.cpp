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

TEST(ScrollController, DragCapturePreservesOffsetUntilPointerMoves)
{
    const ScrollController scroll(12,4,4);
    const Rect track{.x = 7,.y = 3,.width = 1,.height = 4};
    ScrollController::DragState drag;
    int target = -1;
    EXPECT_FALSE(drag.begin(scroll,track,{.x = 6,.y = 4},target));
    EXPECT_FALSE(drag.active());
    EXPECT_TRUE(drag.begin(scroll,track,{.x = 7,.y = 4},target));
    EXPECT_EQ(target,4);
    EXPECT_TRUE(drag.active());
    EXPECT_EQ(drag.move(scroll,track,track.y+track.height),8);
    drag.end();
    EXPECT_FALSE(drag.active());
    EXPECT_EQ(drag.move(scroll,track,track.y),4);
}

TEST(ScrollController, PressingTrackMovesThumb)
{
    const ScrollController scroll(20,4,0);
    const Rect track{.x = 1,.y = 5,.width = 1,.height = 4};
    ScrollController::DragState drag;
    int target = 0;
    EXPECT_TRUE(drag.begin(scroll,track,{.x = 1,.y = 8},target));
    EXPECT_EQ(target,16);
}

TEST(ScrollController, RetainsOffsetAcrossViewportChanges)
{
    ScrollController scroll;
    EXPECT_EQ(scroll.offset(),0);
    scroll.setRequestedOffset(25);
    EXPECT_EQ(scroll.offset(),25);
    scroll.configure(100,10);
    EXPECT_EQ(scroll.offset(),25);
    EXPECT_EQ(scroll.maximum(),90);
    scroll.advance(12);
    EXPECT_EQ(scroll.offset(),37);
    scroll.configure(30,10);
    EXPECT_EQ(scroll.offset(),20);
    scroll.setOffset(-10);
    EXPECT_EQ(scroll.offset(),0);
    scroll.revealItem(29);
    EXPECT_EQ(scroll.offset(),20);
    scroll.configure(5,10);
    EXPECT_EQ(scroll.offset(),0);
    EXPECT_FALSE(scroll.scrollable());
}
