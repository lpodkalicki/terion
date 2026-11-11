/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <terion/terion.h>
#include <gtest/gtest.h>

using namespace terion;

namespace {

int calls = 0;
int started = 0;
int cleaned = 0;

const auto Tracked = component([](int value,bool fail) {
    ++calls;
    auto [count,set_count] = useState(3);
    (void)set_count;
    useEffect([value] {
        ++started;
        return [value] { (void)value; ++cleaned; };
    },value);
    if (fail) throw std::runtime_error("broken render");
    return Typography({.text = std::to_string(count + value)});
});

const auto Replacement = component([] {
    auto [count,set_count] = useState(99);
    (void)set_count;
    return Typography({.text = std::to_string(count)});
});

} /* End of anonymous namespace */

TEST(HookSafety, RejectedDuplicateKeysDoNotInvokeComponents)
{
    calls = 0;
    Reconciler tree;
    EXPECT_THROW(tree.update(Column({.children = {
        Tracked(1,false).keyed("same"),
        Tracked(2,false).keyed("same")
    }})),std::invalid_argument);
    EXPECT_EQ(calls,0);
    EXPECT_EQ(tree.root(),nullptr);
}

TEST(HookSafety, FailedRenderDoesNotCommitEffectOrLoseExistingState)
{
    calls = started = cleaned = 0;
    Reconciler tree;
    tree.update(Tracked(1,false).keyed("tracked"));
    tree.commitEffects();
    ASSERT_NE(tree.root(),nullptr);
    const auto id = tree.root()->id();
    EXPECT_EQ(tree.root()->text(),"4");
    EXPECT_EQ(started,1);

    EXPECT_THROW(tree.update(Tracked(2,true).keyed("tracked")),std::runtime_error);
    EXPECT_EQ(tree.root()->id(),id);
    EXPECT_EQ(tree.root()->text(),"4");
    tree.commitEffects();
    EXPECT_EQ(started,1);
    EXPECT_EQ(cleaned,0);

    tree.update(Tracked(3,false).keyed("tracked"));
    tree.commitEffects();
    EXPECT_EQ(tree.root()->id(),id);
    EXPECT_EQ(tree.root()->text(),"6");
    EXPECT_EQ(started,2);
    EXPECT_EQ(cleaned,1);
}

TEST(HookSafety, FailedReplacementKeepsOldComponentAndCleanup)
{
    calls = started = cleaned = 0;
    Reconciler tree;
    tree.update(Column({.children = {Tracked(1,false).keyed("first")}}));
    tree.commitEffects();
    EXPECT_EQ(started,1);
    EXPECT_THROW(tree.update(Column({.children = {
        Replacement().keyed("first"),
        Tracked(2,true).keyed("second")
    }})),std::runtime_error);
    EXPECT_EQ(cleaned,0);
    EXPECT_EQ(tree.root()->children()[0]->text(),"4");
    tree.update(Column({.children = {Tracked(1,false).keyed("first")}}));
    tree.commitEffects();
    EXPECT_EQ(started,1);
    EXPECT_EQ(cleaned,0);
}

TEST(HookSafety, FailedSiblingRenderRestoresPriorHookRegistry)
{
    std::function<void(int)> set_value;
    const auto stable = component([&] {
        auto [value,set] = useState(3);
        set_value = set;
        return Typography({.text = std::to_string(value)});
    });
    const auto failing = component([](bool fail) {
        auto [value,set] = useState(11);
        (void)set;
        if (fail) throw std::runtime_error("sibling failed");
        return Typography({.text = std::to_string(value)});
    });
    Reconciler tree;
    tree.update(Column({.children = {stable().keyed("stable")}}));
    const auto id = tree.root()->children()[0]->id();
    set_value(42);
    EXPECT_THROW(tree.update(Column({.children = {
        stable().keyed("stable"),failing(true).keyed("new")
    }})),std::runtime_error);
    tree.update(Column({.children = {stable().keyed("stable")}}));
    EXPECT_EQ(tree.root()->children()[0]->id(),id);
    EXPECT_EQ(tree.root()->children()[0]->text(),"42");
}
