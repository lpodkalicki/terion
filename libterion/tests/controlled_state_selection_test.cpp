/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <gtest/gtest.h>
#include <terion/terion.h>
#include <type_traits>

using namespace terion;

static_assert(std::is_aggregate_v<TextField::Props>);
static_assert(std::is_aggregate_v<Table::Props>);
static_assert(std::is_aggregate_v<VirtualTable::Props>);

TEST(ControlledState, TextFieldDefaultValueSurvivesReconciliationAndChangesKeyResets)
{
    Reconciler tree;
    tree.update(TextField({.default_value = "init",.key = "field"}));
    EXPECT_EQ(tree.root()->text(),"init");
    ASSERT_TRUE(tree.root()->handleTextInput({.key = Key::Character,.character = U'!'}));
    EXPECT_EQ(tree.root()->text(),"init!");
    const auto initial_id = tree.root()->id();
    tree.update(TextField({.default_value = "different",.key = "field"}));
    EXPECT_EQ(tree.root()->id(),initial_id);
    EXPECT_EQ(tree.root()->text(),"init!");
    tree.update(TextField({.default_value = "reset",.key = "other"}));
    EXPECT_NE(tree.root()->id(),initial_id);
    EXPECT_EQ(tree.root()->text(),"reset");
}

TEST(ControlledState, ExplicitValueStillWinsOnParentReconciliation)
{
    Reconciler tree;
    std::string changed;
    tree.update(TextField({.value = "fixed",.key = "field",
        .on_change = [&](std::string value) { changed = std::move(value); }}));
    ASSERT_TRUE(tree.root()->handleTextInput({.key = Key::Character,.character = U'!'}));
    EXPECT_EQ(changed,"fixed!");
    tree.update(TextField({.value = "fixed",.key = "field"}));
    EXPECT_EQ(tree.root()->text(),"fixed");
    tree.update(TextField({.value = "parent",.key = "field"}));
    EXPECT_EQ(tree.root()->text(),"parent");
}

TEST(ControlledState, TextAreaAndNumericDefaultsRetainEdits)
{
    Reconciler area;
    area.update(TextArea({.default_value = "first",.key = "area"}));
    ASSERT_TRUE(area.root()->handleTextInput({.key = Key::Enter}));
    area.update(TextArea({.default_value = "changed default",.key = "area"}));
    EXPECT_EQ(area.root()->text(),"first\n");

    Reconciler number;
    number.update(NumberField({.default_value = std::string("12"),.key = "number"}));
    ASSERT_TRUE(number.root()->handleTextInput({.key = Key::Character,.character = U'3'}));
    number.update(NumberField({.default_value = std::string("91"),.key = "number"}));
    EXPECT_EQ(number.root()->text(),"123");
}

TEST(ControlledState, CheckboxAndSwitchDefaultsPersistWithoutCallbacks)
{
    Reconciler box;
    box.update(Checkbox({.key = "box",.default_checked = false}));
    EXPECT_FALSE(box.root()->checked());
    ASSERT_TRUE(box.root()->handleChoiceKey({.key = Key::Enter}));
    EXPECT_TRUE(box.root()->checked());
    box.update(Checkbox({.key = "box",.default_checked = false}));
    EXPECT_TRUE(box.root()->checked());
    box.update(Checkbox({.key = "box",.checked = false}));
    EXPECT_FALSE(box.root()->checked());

    Reconciler toggle;
    toggle.update(Switch({.key = "switch",.default_checked = true}));
    ASSERT_TRUE(toggle.root()->handleChoiceKey({.key = Key::Enter}));
    EXPECT_FALSE(toggle.root()->checked());
    toggle.update(Switch({.key = "switch",.default_checked = true}));
    EXPECT_FALSE(toggle.root()->checked());
}

TEST(ControlledState, SelectAndRadioGroupRememberUncontrolledSelection)
{
    const std::vector<SelectOption> options = {{"Alpha","a"},{"Beta","b"}};
    Reconciler select;
    select.update(Select({.options = options,.default_value = "a",.key = "select"}));
    ASSERT_TRUE(select.root()->handleSelectKey({.key = Key::Enter}));
    ASSERT_TRUE(select.root()->handleSelectKey({.key = Key::Down}));
    ASSERT_TRUE(select.root()->handleSelectKey({.key = Key::Enter}));
    EXPECT_EQ(select.root()->text(),"b");
    select.update(Select({.options = options,.default_value = "a",.key = "select"}));
    EXPECT_EQ(select.root()->text(),"b");

    Reconciler radio;
    radio.update(RadioGroup({.options = {{"Alpha","a"},{"Beta","b"}},
        .default_value = "a",.key = "radio"}));
    ASSERT_TRUE(radio.root()->handleChoiceKey({.key = Key::Right}));
    EXPECT_EQ(radio.root()->text(),"b");
    radio.update(RadioGroup({.options = {{"Alpha","a"},{"Beta","b"}},
        .default_value = "a",.key = "radio"}));
    EXPECT_EQ(radio.root()->text(),"b");
}

TEST(ControlledState, FormRemainsAuthoritativeOverNestedDefaultValues)
{
    const auto create = [] {
        return Form({.children = {
            FormField({.name = "username",.control = TextField({.default_value = "Alice"})})
        }});
    };
    Reconciler tree;
    tree.update(create());
    // FormField binds a controlled input into FormState.
    auto* field = tree.root()->children()[0]->children()[0].get();
    ASSERT_EQ(field->text(),"Alice");
    ASSERT_TRUE(field->handleTextInput({.key = Key::Character,.character = U'!'}));
    tree.update(create());
    EXPECT_EQ(tree.root()->children()[0]->children()[0]->text(),"Alice!");
    tree.root()->children().back()->children()[1]->onClick()(); // Reset
    tree.update(create());
    EXPECT_EQ(tree.root()->children()[0]->children()[0]->text(),"Alice");
}

TEST(SelectionApi, TableUsesCanonicalKeyAndCallback)
{
    Reconciler tree;
    std::string selected;
    tree.update(Table({.columns = {{"Name",12}},.rows = {
        {"a",{"Alpha"}},{"b",{"Beta"}}},
        .selected_key = "a",.on_selection_change = [&](std::string key) {
            selected = std::move(key);
        }}));
    tree.root()->layout({.maximum = {.width = 20,.height = 5}});
    ASSERT_TRUE(tree.root()->handleTableKey({.key = Key::Down}));
    EXPECT_EQ(selected,"b");
}

TEST(SelectionApi, TreeAndTabsAcceptSelectedKey)
{
    const TreeView tree({.items = {{.key = "node",.label = "Node"}},.selected_key = "node"});
    const Tabs tabs({.options = {{"General","general"}},.selected_key = "general"});
    EXPECT_EQ(tree.props().text,"node");
    EXPECT_EQ(tabs.props().text,"general");
}

TEST(SelectionApi, VirtualListUsesSingleSelectionCallback)
{
    int calls = 0;
    int selected = -1;
    Reconciler tree;
    tree.update(VirtualList({.item_count = 5,.selected_index = 0,
        .on_selection_change = [&](int index) { selected = index; ++calls; }}));
    tree.root()->layout({.maximum = {.width = 12,.height = 4}});
    ASSERT_TRUE(tree.root()->handleVirtualListKey({.key = Key::Down}));
    EXPECT_EQ(selected,1);
    EXPECT_EQ(calls,1);
}

TEST(SelectionApi, VirtualTablePreferredKeyCallbackAndLazyRecords)
{
    int loaded = 0;
    std::string selected_key;
    Reconciler tree;
    tree.update(VirtualTable({.columns = {{"Name",10}},.row_count = 10,
        .row_at = [&](int index) {
            ++loaded;
            return TableRow{.key = "id-"+std::to_string(index),.cells = {"Item"}};
        },
        .row_key_at = [](int index) { return "id-"+std::to_string(index); },
        .selected_index = -1,.selected_key = "id-1",
        .on_selection_key_change = [&](std::string key) { selected_key = std::move(key); },
        .layout = {.width = 12,.height = 5}}));
    EXPECT_EQ(loaded,0);
    auto* list = tree.root()->children()[1].get();
    list->layout({.maximum = {.width = 12,.height = 4}});
    ASSERT_TRUE(list->handleVirtualListClick({.x = 0,.y = 2}));
    EXPECT_EQ(selected_key,"id-2");
    EXPECT_LE(loaded,1);
}
