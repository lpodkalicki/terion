/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <terion/terion.h>
#include <gtest/gtest.h>

using namespace terion;

TEST(UseId, StableAcrossFramesAndKeyedReorder)
{
    const auto item = component([] {
        return Typography({.text = useId()});
    });
    const auto render = [&](bool reversed) {
        return reversed
            ? Column({.children = {item().keyed("b"),item().keyed("a")}})
            : Column({.children = {item().keyed("a"),item().keyed("b")}});
    };

    Reconciler tree;
    tree.update(render(false));
    const auto a = tree.root()->children()[0]->text();
    const auto b = tree.root()->children()[1]->text();
    EXPECT_NE(a,b);
    EXPECT_EQ(a.rfind("terion-",0),0u);
    tree.update(render(true));
    EXPECT_EQ(tree.root()->children()[0]->text(),b);
    EXPECT_EQ(tree.root()->children()[1]->text(),a);
}

TEST(UseId, MultipleSlotsAndRemountReceiveDistinctIds)
{
    const auto item = component([] {
        const auto first = useId();
        const auto second = useId();
        return Typography({.text = first + "," + second});
    });
    Reconciler tree;
    tree.update(Column({.children = {item().keyed("item")}}));
    const auto first_mount = tree.root()->children()[0]->text();
    const auto separator = first_mount.find(',');
    ASSERT_NE(separator,std::string::npos);
    EXPECT_NE(first_mount.substr(0,separator),first_mount.substr(separator + 1));

    tree.update(Column({.children = {item().keyed("item")}}));
    EXPECT_EQ(tree.root()->children()[0]->text(),first_mount);

    tree.update(Column({.children = {}}));
    tree.update(Column({.children = {item().keyed("item")}}));
    EXPECT_NE(tree.root()->children()[0]->text(),first_mount);
}

TEST(UseId, RequiresMountAndStableHookKind)
{
    EXPECT_THROW((void)useId(),std::logic_error);
    const auto mixed = component([](bool with_id) {
        if (with_id) (void)useId();
        else (void)useState(std::string("state"));
        return Typography({.text = "ok"});
    });
    Reconciler tree;
    tree.update(mixed(true).keyed("mixed"));
    EXPECT_THROW(tree.update(mixed(false).keyed("mixed")),std::logic_error);
}
