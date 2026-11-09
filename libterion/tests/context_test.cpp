/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <terion/terion.h>
#include <gtest/gtest.h>

using namespace terion;

namespace {

const Context<std::string> context("default");

const auto Reader = component([] {
    return Typography({.text = useContext(context)});
});

const auto Middle = component([] {
    return Reader().keyed("reader");
});

} /* End of anonymous namespace */

TEST(Context, FallsBackAndOverridesWithNearestProvider)
{
    Reconciler reconciler;
    reconciler.update(Column({.children = {
        Reader().keyed("outside"),
        ContextProvider(context,std::string("outer"),Column({.children = {
            Middle().keyed("middle"),
            ContextProvider(context,std::string("inner"),Reader().keyed("nested"),"nested-provider")
        }}),"outer-provider")
    }}));
    const auto& children = reconciler.root()->children();
    ASSERT_EQ(children.size(),2u);
    EXPECT_EQ(children[0]->text(),"default");
    ASSERT_EQ(children[1]->children().size(),2u);
    EXPECT_EQ(children[1]->children()[0]->text(),"outer");
    EXPECT_EQ(children[1]->children()[1]->text(),"inner");
}

TEST(Context, UpdatesOnProviderChange)
{
    Reconciler reconciler;
    const auto build = [](std::string value) {
        return ContextProvider(context,std::move(value),Reader().keyed("reader"),"provider");
    };
    reconciler.update(build("light"));
    const auto id = reconciler.root()->id();
    EXPECT_EQ(reconciler.root()->text(),"light");
    reconciler.update(build("dark"));
    EXPECT_EQ(reconciler.root()->id(),id);
    EXPECT_EQ(reconciler.root()->text(),"dark");
}

TEST(Context, RequiresMountedFunction)
{
    EXPECT_THROW((void)useContext(context),std::logic_error);
}

TEST(Context, CallableComponentsInheritNearestProvider)
{
    const auto reader = component([] {
        return Typography({.text = useContext(context)});
    });
    const auto middle = component([&] {
        return Column({.children = {reader()}});
    });

    Reconciler reconciler;
    reconciler.update(Column({.children = {
        reader(),
        ContextProvider(context,std::string("outer"),Column({.children = {
            middle(),
            ContextProvider(context,std::string("inner"),reader())
        }}))
    }}));
    const auto& children = reconciler.root()->children();
    EXPECT_EQ(children[0]->text(),"default");
    EXPECT_EQ(children[1]->children()[0]->children()[0]->text(),"outer");
    EXPECT_EQ(children[1]->children()[1]->text(),"inner");
}

TEST(Context, MultipleDirectChildrenShareScopeWithoutLayoutNode)
{
    Reconciler tree;
    tree.update(Column({.children = {
        ContextProvider(context,std::string("shared"),std::vector<Component>{
            Reader().keyed("a"),Reader().keyed("b")
        })
    }}));
    ASSERT_EQ(tree.root()->children().size(),2u);
    EXPECT_EQ(tree.root()->children()[0]->text(),"shared");
    EXPECT_EQ(tree.root()->children()[1]->text(),"shared");
}

TEST(Context, StatefulProviderUpdatesDescendantsThroughContext)
{
    struct Model
    {
        bool dark;
        std::function<void()> toggle;
    };
    const Context<Model> theme({false,{}});
    const auto useTheme = [&] { return useContext(theme); };
    const auto label = component([&] {
        return Typography({.text = useTheme().dark ? "dark" : "light"});
    });
    const auto toggle = component([&] {
        return Button({.text = "toggle",.onClick = useTheme().toggle});
    });
    const auto provider = component([&] {
        auto [dark,setDark] = useState(false);
        return ContextProvider(theme,Model{dark,[setDark] {
            setDark([](bool previous) { return !previous; });
        }},std::vector<Component>{label(),toggle()});
    });

    Reconciler tree;
    tree.update(Column({.children = {provider()}}));
    EXPECT_EQ(tree.root()->children()[0]->text(),"light");
    tree.root()->children()[1]->onClick()();
    tree.update(Column({.children = {provider()}}));
    EXPECT_EQ(tree.root()->children()[0]->text(),"dark");
}
