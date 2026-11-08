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

Component Tracked(int value,bool fail)
{
    ++calls;
    auto [count,setCount] = useState(3);
    (void)setCount;
    useEffect([value] {
        ++started;
        return [value] { (void)value; ++cleaned; };
    },value);
    if (fail) throw std::runtime_error("broken render");
    return Typography({.text = std::to_string(count + value)});
}

Component Replacement()
{
    auto [count,setCount] = useState(99);
    (void)setCount;
    return Typography({.text = std::to_string(count)});
}

} /* End of anonymous namespace */

TEST(HookSafety, RejectedDuplicateKeysDoNotInvokeComponents)
{
    calls = 0;
    Reconciler tree;
    EXPECT_THROW(tree.update(Column({.children = {
        ComponentOf<Tracked>("same",1,false),
        ComponentOf<Tracked>("same",2,false)
    }})),std::invalid_argument);
    EXPECT_EQ(calls,0);
    EXPECT_EQ(tree.root(),nullptr);
}

TEST(HookSafety, FailedRenderDoesNotCommitEffectOrLoseExistingState)
{
    calls = started = cleaned = 0;
    Reconciler tree;
    tree.update(ComponentOf<Tracked>("tracked",1,false));
    tree.commitEffects();
    ASSERT_NE(tree.root(),nullptr);
    const auto id = tree.root()->id();
    EXPECT_EQ(tree.root()->text(),"4");
    EXPECT_EQ(started,1);

    EXPECT_THROW(tree.update(ComponentOf<Tracked>("tracked",2,true)),std::runtime_error);
    EXPECT_EQ(tree.root()->id(),id);
    EXPECT_EQ(tree.root()->text(),"4");
    tree.commitEffects();
    EXPECT_EQ(started,1);
    EXPECT_EQ(cleaned,0);

    tree.update(ComponentOf<Tracked>("tracked",3,false));
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
    tree.update(Column({.children = {ComponentOf<Tracked>("first",1,false)}}));
    tree.commitEffects();
    EXPECT_EQ(started,1);
    EXPECT_THROW(tree.update(Column({.children = {
        ComponentOf<Replacement>("first"),
        ComponentOf<Tracked>("second",2,true)
    }})),std::runtime_error);
    EXPECT_EQ(cleaned,0);
    EXPECT_EQ(tree.root()->children()[0]->text(),"4");
    tree.update(Column({.children = {ComponentOf<Tracked>("first",1,false)}}));
    tree.commitEffects();
    EXPECT_EQ(started,1);
    EXPECT_EQ(cleaned,0);
}
