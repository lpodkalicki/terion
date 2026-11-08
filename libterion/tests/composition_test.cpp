/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <terion/terion.h>
#include <gtest/gtest.h>

using namespace terion;

namespace {

struct LabelProps
{
    std::string text;
    std::string key;
};

Component Label(LabelProps props)
{
    return Typography({.text = std::move(props.text),.key = std::move(props.key)});
}

} /* End of anonymous namespace */

TEST(ComponentComposition, PlainFunctionsPreserveKeyedIdentity)
{
    Reconciler reconciler;
    reconciler.update(Column({.children = {Label({"A","a"}),Label({"B","b"})}}));
    ASSERT_EQ(reconciler.root()->children().size(),2u);
    const auto first_id = reconciler.root()->children()[0]->id();
    const auto second_id = reconciler.root()->children()[1]->id();
    reconciler.update(Column({.children = {Label({"Updated B","b"}),Label({"Updated A","a"})}}));
    EXPECT_EQ(reconciler.root()->children()[0]->id(),second_id);
    EXPECT_EQ(reconciler.root()->children()[1]->id(),first_id);
    EXPECT_EQ(reconciler.root()->children()[0]->text(),"Updated B");
}

TEST(ComponentComposition, ComposesChildrenAndCallbacks)
{
    struct PanelProps
    {
        std::vector<Component> children;
        std::function<void()> onClick;
    };
    const auto Panel = [](PanelProps props) -> Component {
        props.children.emplace_back(Button({.text = "Action",.key = "action",
            .onClick = std::move(props.onClick)}));
        return Column({.children = std::move(props.children)});
    };
    bool invoked = false;
    auto panel = Panel({.children = {Label({"Body","body"})},
        .onClick = [&] { invoked = true; }});
    ASSERT_EQ(panel.props().children.size(),2u);
    EXPECT_EQ(panel.props().children[0].props().key,"body");
    panel.props().children[1].props().on_click();
    EXPECT_TRUE(invoked);
}
