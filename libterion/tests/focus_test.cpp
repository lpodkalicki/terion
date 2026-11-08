/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <terion/terion.h>
#include <gtest/gtest.h>

using namespace terion;

TEST(Focus, CyclesThroughRegisteredHandlers)
{
    FocusManager focus;
    int first_calls = 0;
    int second_calls = 0;
    focus.add("first",[&](const Event&) { ++first_calls; return true; });
    focus.add("second",[&](const Event&) { ++second_calls; return true; });
    EXPECT_EQ(focus.current(),"first");
    EXPECT_TRUE(focus.dispatch(Event{.key = Key::Character,.character = U'x'}));
    EXPECT_TRUE(first_calls == 1 && second_calls == 0);
    EXPECT_TRUE(focus.dispatch(Event{.key = Key::Tab}));
    EXPECT_EQ(focus.current(),"second");
    EXPECT_TRUE(focus.dispatch(Event{.key = Key::Enter}));
    EXPECT_EQ(second_calls,1);
    EXPECT_TRUE(focus.previous() && focus.current() == "first");
    EXPECT_TRUE(focus.remove("first") && focus.current() == "second");
    EXPECT_TRUE(!focus.focus("missing"));

}

TEST(Focus, BlurStopsKeyboardDispatchUntilNextFocus)
{
    FocusManager focus;
    int calls = 0;
    focus.add("first",[&](const Event&) { ++calls; return true; });
    focus.add("second",[&](const Event&) { ++calls; return true; });
    focus.blur();
    EXPECT_TRUE(focus.current().empty());
    EXPECT_FALSE(focus.dispatch(Event{.key = Key::Character,.character = U'x'}));
    EXPECT_EQ(calls,0);
    EXPECT_TRUE(focus.dispatch(Event{.key = Key::Tab}));
    EXPECT_EQ(focus.current(),"first");
    focus.blur();
    EXPECT_TRUE(focus.previous());
    EXPECT_EQ(focus.current(),"second");
}
