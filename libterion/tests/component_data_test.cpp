/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <gtest/gtest.h>
#include <terion/terion.h>
#include <type_traits>

using namespace terion;

static_assert(std::is_aggregate_v<Component::Props>);
static_assert(std::is_aggregate_v<Component::TextData>);
static_assert(std::is_aggregate_v<Component::TableData>);
static_assert(std::is_aggregate_v<Component::VirtualListData>);

TEST(ComponentData, TextControlStoresOnlyRelevantGroups)
{
    const Button button({.text = "Save",.id = "save",.on_click = [] {}});
    const auto& props = button.props();
    EXPECT_EQ(props.id,"save");
    ASSERT_TRUE(props.text);
    ASSERT_TRUE(props.action);
    ASSERT_TRUE(props.input);
    EXPECT_FALSE(props.table);
    EXPECT_FALSE(props.tree);
    EXPECT_FALSE(props.virtual_list);
    EXPECT_EQ(props.textData().text,"Save");
    EXPECT_TRUE(static_cast<bool>(props.actionData().on_click));
}

TEST(ComponentData, ReadOnlyDefaultsDoNotMaterializeUnusedGroups)
{
    Component text = Component::text("Hello");
    const auto& props = text.props();
    ASSERT_TRUE(props.text);
    EXPECT_FALSE(props.tree);
    EXPECT_TRUE(props.treeData().tree_items.empty());
    EXPECT_FALSE(props.tree);
    EXPECT_EQ(props.textData().text,"Hello");
}

TEST(ComponentData, MutablePayloadIsIndependentAcrossCopies)
{
    const TreeView tree({.items = {{.key = "root",.label = "Root"}}});
    auto copied = tree.props();
    copied.treeData().tree_items.front().label = "Modified";
    EXPECT_EQ(tree.props().treeData().tree_items.front().label,"Root");
    EXPECT_EQ(copied.treeData().tree_items.front().label,"Modified");
}

TEST(ComponentData, ReconcilerPreservesControlledAndUncontrolledValues)
{
    Reconciler reconciler;
    reconciler.update(TextField({.default_value = "A",.key = "field"}));
    ASSERT_TRUE(reconciler.root()->handleTextInput({.key = Key::Character,.character = U'!'}));
    reconciler.update(TextField({.default_value = "B",.key = "field"}));
    EXPECT_EQ(reconciler.root()->text(),"A!");
    reconciler.update(TextField({.default_value = "C",.key = "another"}));
    EXPECT_EQ(reconciler.root()->text(),"C");
}

TEST(ComponentData, TableHasDedicatedPayloadAndSelection)
{
    const Table table({.columns = {{"Name",12}},.rows = {{"a",{"Alpha"}}},
        .selected_key = "a"});
    EXPECT_TRUE(table.props().table.has_value());
    EXPECT_EQ(table.props().tableData().table_columns.size(),1U);
    EXPECT_EQ(table.props().textData().text,"a");
    EXPECT_FALSE(table.props().tree.has_value());
}

TEST(ComponentData, VirtualListOnlyAllocatesItsPayload)
{
    const VirtualList list({.item_count = 500,.row_height = 2});
    EXPECT_EQ(list.props().virtualListData().virtual_item_count,500);
    EXPECT_EQ(list.props().virtualListData().virtual_row_height,2);
    EXPECT_FALSE(list.props().table.has_value());
    EXPECT_FALSE(list.props().tree.has_value());
}

TEST(ComponentData, FormStateStillOwnsNestedInputs)
{
    auto create = [] {
        return Form({.children = {
            FormField({.name = "username",.control = TextField({.default_value = "Alice"})})
        }});
    };
    Reconciler reconciler;
    reconciler.update(create());
    auto* field = reconciler.root()->children()[0]->children()[0].get();
    ASSERT_TRUE(field->handleTextInput({.key = Key::Character,.character = U'!'}));
    reconciler.update(create());
    EXPECT_EQ(reconciler.root()->children()[0]->children()[0]->text(),"Alice!");
}

TEST(ComponentData, PublicIdAndReconciliationKeyRemainSeparate)
{
    const TextField field({.value = "Hello",.key = "reconcile-key",.id = "focus-id"});
    EXPECT_EQ(field.props().key,"reconcile-key");
    EXPECT_EQ(field.props().id,"focus-id");
    EXPECT_EQ(field.props().textData().text,"Hello");
}
