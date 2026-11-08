/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <terion/terion.h>
#include <gtest/gtest.h>

using namespace terion;

namespace {

Component Counter()
{
    auto [count,setCount] = useState(0);
    return Button({.text = std::to_string(count),
        .onClick = [setCount] { setCount([](int previous) { return previous + 1; }); }});
}

Component BadHookOrder(bool second)
{
    auto [count,setCount] = useState(0);
    (void)setCount;
    if (second) (void)useState(std::string("extra"));
    return Typography({.text = std::to_string(count)});
}

} /* End of anonymous namespace */

TEST(Hooks, IndependentInstancesAndKeyedReordering)
{
    Reconciler reconciler;
    const auto build = [](bool reversed) {
        return reversed
            ? Column({.children = {ComponentOf<Counter>("b"),ComponentOf<Counter>("a")}})
            : Column({.children = {ComponentOf<Counter>("a"),ComponentOf<Counter>("b")}});
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
    reconciler.update(Column({.children = {ComponentOf<Counter>("counter")}}));
    reconciler.root()->children()[0]->onClick()();
    reconciler.update(Column({.children = {ComponentOf<Counter>("counter")}}));
    EXPECT_EQ(reconciler.root()->children()[0]->text(),"1");
    reconciler.update(Column({.children = {}}));
    reconciler.update(Column({.children = {ComponentOf<Counter>("counter")}}));
    EXPECT_EQ(reconciler.root()->children()[0]->text(),"0");
}

TEST(Hooks, RequiresMountAndStableHookOrder)
{
    EXPECT_THROW((void)Counter(),std::logic_error);
    Reconciler reconciler;
    reconciler.update(ComponentOf<BadHookOrder>("bad",false));
    EXPECT_THROW(reconciler.update(ComponentOf<BadHookOrder>("bad",true)),std::logic_error);
    // A rejected render must leave the previous valid hook count intact.
    EXPECT_NO_THROW(reconciler.update(ComponentOf<BadHookOrder>("bad",false)));

    Reconciler another;
    another.update(ComponentOf<BadHookOrder>("bad",true));
    EXPECT_THROW(another.update(ComponentOf<BadHookOrder>("bad",false)),std::logic_error);
}
