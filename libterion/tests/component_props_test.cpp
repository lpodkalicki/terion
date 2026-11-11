/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <terion/terion.h>
#include <gtest/gtest.h>

using namespace terion;

namespace {

struct PanelProps
{
    std::string title = {};
    std::vector<std::string> names = {};
};

const auto Panel = component([](const PanelProps& props) {
    return Typography({.text = props.title + ":" + std::to_string(props.names.size())});
});

} /* End of anonymous namespace */

TEST(ComponentProps, CopiesLvaluePropsIntoDeferredDeclaration)
{
    PanelProps props{"Original",{"first","second"}};
    auto declaration = Panel(props);
    props.title = "Modified";
    props.names.clear();

    Reconciler tree;
    tree.update(declaration);
    EXPECT_EQ(tree.root()->text(),"Original:2");
}

TEST(ComponentProps, SupportsTemporaryDesignatedInitializers)
{
    auto declaration = Panel({.title = "Temporary",.names = {"one"}}).keyed("panel");
    Reconciler tree;
    tree.update(declaration);
    EXPECT_EQ(tree.root()->text(),"Temporary:1");
}

TEST(ComponentProps, ChangingPropsKeepsStateForSameKey)
{
    const auto counter = component([](const PanelProps& props) {
        auto [count,set_count] = useState(0);
        return Button({.text = props.title + std::to_string(count),
            .on_click = [set_count] { set_count([](int n) { return n + 1; }); }});
    });
    Reconciler tree;
    tree.update(counter({.title = "A"}).keyed("counter"));
    tree.root()->onClick()();
    tree.update(counter({.title = "B"}).keyed("counter"));
    EXPECT_EQ(tree.root()->text(),"B1");
}
