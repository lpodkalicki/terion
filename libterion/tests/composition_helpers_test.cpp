/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <terion/terion.h>
#include <gtest/gtest.h>

using namespace terion;

TEST(CompositionHelpers, WhenMountsOnlyVisibleChild)
{
    int calls = 0;
    const auto panel = component([&] {
        ++calls;
        return Typography({.text = "panel"});
    });
    Reconciler tree;
    tree.update(Column({.children = {When(false,panel())}}));
    EXPECT_EQ(calls,0);
    EXPECT_TRUE(tree.root()->children().empty());
    tree.update(Column({.children = {When(true,panel())}}));
    EXPECT_EQ(calls,1);
    ASSERT_EQ(tree.root()->children().size(),1u);
    EXPECT_EQ(tree.root()->children()[0]->text(),"panel");
    tree.update(Column({.children = {When(false,panel())}}));
    EXPECT_TRUE(tree.root()->children().empty());
}

TEST(CompositionHelpers, ForEachKeepsKeyedStateOnReorder)
{
    const auto counter = component([](std::string name) {
        auto [count,set_count] = useState(0);
        return Button({.text = name + std::to_string(count),
            .on_click = [set_count] { set_count([](int n) { return n + 1; }); }});
    });
    const auto build = [&](const std::vector<std::string>& names) {
        return Column({.children = {ForEach(names,[&](const std::string& name) {
            return counter(name).keyed(name);
        })}});
    };
    Reconciler tree;
    tree.update(build({"a","b"}));
    tree.root()->children()[0]->onClick()();
    tree.update(build({"b","a"}));
    ASSERT_EQ(tree.root()->children().size(),2u);
    EXPECT_EQ(tree.root()->children()[0]->text(),"b0");
    EXPECT_EQ(tree.root()->children()[1]->text(),"a1");
}

TEST(CompositionHelpers, ForEachRejectsMissingAndDuplicateKeys)
{
    const std::vector<std::string> names{"a","a"};
    EXPECT_THROW((void)ForEach(names,[](const auto& name) {
        return Typography({.text = name});
    }),std::invalid_argument);
    EXPECT_THROW((void)ForEach(names,[](const auto& name) {
        return Typography({.text = name,.key = name});
    }),std::invalid_argument);
}
