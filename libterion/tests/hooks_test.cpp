/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <terion/terion.h>
#include <gtest/gtest.h>

using namespace terion;

namespace {

const auto Counter = component([] {
    auto [count,setCount] = useState(0);
    return Button({.text = std::to_string(count),
        .onClick = [setCount] { setCount([](int previous) { return previous + 1; }); }});
});

const auto BadHookOrder = component([](bool second) {
    auto [count,setCount] = useState(0);
    (void)setCount;
    if (second) (void)useState(std::string("extra"));
    return Typography({.text = std::to_string(count)});
});

} /* End of anonymous namespace */

TEST(Hooks, IndependentInstancesAndKeyedReordering)
{
    Reconciler reconciler;
    const auto build = [](bool reversed) {
        return reversed
            ? Column({.children = {Counter().keyed("b"),Counter().keyed("a")}})
            : Column({.children = {Counter().keyed("a"),Counter().keyed("b")}});
    };
    reconciler.update(build(false));
    ASSERT_EQ(reconciler.root()->children().size(),2u);
    EXPECT_EQ(reconciler.root()->children()[0]->text(),"0");
    reconciler.root()->children()[0]->onClick()();
    reconciler.update(build(true));
    EXPECT_EQ(reconciler.root()->children()[0]->text(),"0");
    EXPECT_EQ(reconciler.root()->children()[1]->text(),"1");
}

TEST(Hooks, UnmountDiscardsState)
{
    Reconciler reconciler;
    reconciler.update(Column({.children = {Counter().keyed("counter")}}));
    reconciler.root()->children()[0]->onClick()();
    reconciler.update(Column({.children = {Counter().keyed("counter")}}));
    EXPECT_EQ(reconciler.root()->children()[0]->text(),"1");
    reconciler.update(Column({.children = {}}));
    reconciler.update(Column({.children = {Counter().keyed("counter")}}));
    EXPECT_EQ(reconciler.root()->children()[0]->text(),"0");
}

TEST(Hooks, RequiresMountAndStableHookOrder)
{
    EXPECT_NO_THROW((void)Counter());
    EXPECT_THROW((void)useState(0),std::logic_error);
    Reconciler reconciler;
    reconciler.update(BadHookOrder(false).keyed("bad"));
    EXPECT_THROW(reconciler.update(BadHookOrder(true).keyed("bad")),std::logic_error);
    // A rejected render must leave the previous valid hook count intact.
    EXPECT_NO_THROW(reconciler.update(BadHookOrder(false).keyed("bad")));

    Reconciler another;
    another.update(BadHookOrder(true).keyed("bad"));
    EXPECT_THROW(another.update(BadHookOrder(false).keyed("bad")),std::logic_error);
}

TEST(Hooks, CallableComponentsPreserveKeyedState)
{
    const auto counter = component([] {
        auto [count,setCount] = useState(0);
        return Button({.text = std::to_string(count),
            .onClick = [setCount] { setCount([](int value) { return value + 1; }); }});
    });

    Reconciler reconciler;
    auto build = [&](bool reverse) {
        return reverse
            ? Column({.children = {counter().keyed("b"),counter().keyed("a")}})
            : Column({.children = {counter().keyed("a"),counter().keyed("b")}});
    };
    reconciler.update(build(false));
    reconciler.root()->children()[0]->onClick()();
    reconciler.update(build(true));
    EXPECT_EQ(reconciler.root()->children()[0]->text(),"0");
    EXPECT_EQ(reconciler.root()->children()[1]->text(),"1");
}
