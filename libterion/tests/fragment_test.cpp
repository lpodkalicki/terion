/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <terion/terion.h>
#include <gtest/gtest.h>

using namespace terion;

TEST(Fragment, FlattensChildrenWithoutCreatingLayoutNode)
{
    Reconciler tree;
    tree.update(Column({.children = {
        Typography({.text = "before"}),
        Fragment({.children = {
            Typography({.text = "one"}),
            Fragment({.children = {Typography({.text = "two"})}})
        }}),
        Typography({.text = "after"})
    }}));
    ASSERT_EQ(tree.root()->children().size(),4u);
    EXPECT_EQ(tree.root()->children()[0]->text(),"before");
    EXPECT_EQ(tree.root()->children()[1]->text(),"one");
    EXPECT_EQ(tree.root()->children()[2]->text(),"two");
    EXPECT_EQ(tree.root()->children()[3]->text(),"after");
    EXPECT_EQ(tree.stats().visited,5u);
}

TEST(Fragment, PreservesFunctionHooksAndProviderScope)
{
    const Context<std::string> theme("fallback");
    const auto label = component([&] {
        auto [count,setCount] = useState(0);
        return Button({.text = useContext(theme) + std::to_string(count),
            .onClick = [setCount] { setCount([](int n) { return n + 1; }); }});
    });
    const auto build = [&] {
        return Column({.children = {
            ContextProvider(theme,std::string("dark"),Fragment({.children = {
                label().keyed("label")
            }}))
        }});
    };
    Reconciler tree;
    tree.update(build());
    ASSERT_EQ(tree.root()->children().size(),1u);
    EXPECT_EQ(tree.root()->children()[0]->text(),"dark0");
    tree.root()->children()[0]->onClick()();
    tree.update(build());
    EXPECT_EQ(tree.root()->children()[0]->text(),"dark1");
}

TEST(Fragment, RootFragmentUsesSingleLayoutHost)
{
    Reconciler tree;
    tree.update(Fragment({.children = {
        Typography({.text = "a"}),Typography({.text = "b"})
    }}));
    ASSERT_NE(tree.root(),nullptr);
    EXPECT_EQ(tree.root()->type(),Component::Type::Column);
    EXPECT_EQ(tree.root()->children().size(),2u);
}
