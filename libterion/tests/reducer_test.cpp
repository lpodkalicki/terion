/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <terion/terion.h>
#include <gtest/gtest.h>

using namespace terion;

namespace {

enum class Action { Increment, Decrement, Reset };

int reduceCount(int value,Action action)
{
    switch (action)
    {
        case Action::Increment: return value + 1;
        case Action::Decrement: return value - 1;
        case Action::Reset: return 0;
    }
    return value;
}

const auto Counter = component([] {
    auto [count,dispatch] = useReducer(reduceCount,0);
    return Button({.text = std::to_string(count),
        .onClick = [dispatch] { dispatch(Action::Increment); }});
});

} /* End of anonymous namespace */

TEST(Reducer, DispatchUpdatesLatestStateAcrossRenders)
{
    Reconciler tree;
    tree.update(Counter().keyed("counter"));
    const auto onClick = tree.root()->onClick();
    onClick();
    onClick();
    tree.update(Counter().keyed("counter"));
    EXPECT_EQ(tree.root()->text(),"2");
    tree.root()->onClick()();
    tree.update(Counter().keyed("counter"));
    EXPECT_EQ(tree.root()->text(),"3");
}

TEST(Reducer, UnmountResetsReducerState)
{
    Reconciler tree;
    tree.update(Column({.children = {Counter().keyed("counter")}}));
    tree.root()->children()[0]->onClick()();
    tree.update(Column({.children = {}}));
    tree.update(Column({.children = {Counter().keyed("counter")}}));
    EXPECT_EQ(tree.root()->children()[0]->text(),"0");
}

TEST(Reducer, RejectsCallsOutsideMountAndChangedHookKinds)
{
    EXPECT_THROW((void)useReducer(reduceCount,0),std::logic_error);
    const auto variant = component([](bool reducer) {
        if (reducer) (void)useReducer(reduceCount,0);
        else (void)useState(0);
        return Typography({.text = "ok"});
    });
    Reconciler tree;
    tree.update(variant(true).keyed("variant"));
    EXPECT_THROW(tree.update(variant(false).keyed("variant")),std::logic_error);
}
