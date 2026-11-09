/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <terion/state.h>
#include <gtest/gtest.h>

using namespace terion;

TEST(StateReentrancy, RejectsRecursiveAccessWithoutDeadlock)
{
    State<int> counter(1);
    EXPECT_THROW(counter.update([&](int value) { return value+counter.get(); }),std::logic_error);
    EXPECT_EQ(counter.get(),1);
    counter.update([](int value) { return value+1; });
    EXPECT_EQ(counter.get(),2);
}

TEST(StateReentrancy, NestedUpdatesOfDifferentStatesRemainValid)
{
    State<int> first(1);
    State<int> second(2);
    first.update([&](int value) {
        second.update([](int other) { return other+1; });
        return value+1;
    });
    EXPECT_EQ(first.get(),2);
    EXPECT_EQ(second.get(),3);
}
