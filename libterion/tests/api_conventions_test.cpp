/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <terion/terion.h>
#include <gtest/gtest.h>
#include <type_traits>
#include <utility>

using namespace terion;

static_assert(std::is_same_v<ControlSize,ButtonSize>);
static_assert(std::is_same_v<InputVariant,TextFieldVariant>);
static_assert(std::is_aggregate_v<Button::Props>);
static_assert(std::is_aggregate_v<TextField::Props>);
static_assert(std::is_same_v<decltype(Button::Props{}.size),ControlSize>);
static_assert(std::is_same_v<decltype(Select::Props{}.size),ControlSize>);
static_assert(std::is_same_v<decltype(Autocomplete::Props{}.size),ControlSize>);
static_assert(std::is_same_v<decltype(DropdownMenu::Props{}.size),ControlSize>);
static_assert(std::is_same_v<decltype(TextField::Props{}.size),ControlSize>);
static_assert(std::is_same_v<decltype(NumberField::Props{}.size),ControlSize>);
static_assert(std::is_same_v<decltype(PasswordField::Props{}.size),ControlSize>);
static_assert(std::is_same_v<decltype(TextField::Props{}.variant),InputVariant>);
static_assert(std::is_same_v<decltype(NumberField::Props{}.variant),InputVariant>);
static_assert(std::is_same_v<decltype(PasswordField::Props{}.variant),InputVariant>);
static_assert(std::is_same_v<decltype(Button::Props{}.on_click),std::function<void()>>);
static_assert(std::is_same_v<decltype(TextField::Props{}.on_change),
                             std::function<void(std::string)>>);
static_assert(std::is_same_v<decltype(VirtualList::Props{}.on_activate),
                             std::function<void(int)>>);

TEST(ApiConventions, LegacyAndPreferredControlSizesAreEquivalent)
{
    const Theme theme = Theme::dark();
    EXPECT_EQ(theme.controlMetrics(ControlSize::Compact),
              theme.controlMetrics(ButtonSize::Compact));
    EXPECT_EQ(theme.controlMetrics(ControlSize::Medium),
              theme.controlMetrics(ButtonSize::Medium));
    EXPECT_EQ(theme.controlMetrics(ControlSize::Large),
              theme.controlMetrics(ButtonSize::Large));

    const Button legacy({.text = "OK",.size = ButtonSize::Compact});
    const Button preferred({.text = "OK",.size = ControlSize::Compact});
    EXPECT_EQ(legacy.props().button_size,preferred.props().button_size);

    const Select select({.size = ControlSize::Large});
    const Autocomplete autocomplete({.size = ButtonSize::Large});
    EXPECT_EQ(select.props().button_size,autocomplete.props().button_size);
}

TEST(ApiConventions, InputVariantRetainsLegacyRenderingContract)
{
    const TextField old_field({.variant = TextFieldVariant::Filled,
                               .size = ButtonSize::Medium});
    const TextField new_field({.variant = InputVariant::Filled,
                               .size = ControlSize::Medium});
    EXPECT_EQ(old_field.props().text_field_variant,new_field.props().text_field_variant);
    EXPECT_EQ(old_field.props().button_size,new_field.props().button_size);

    const NumberField number({.value = 42.0,.variant = InputVariant::Outlined,
                               .size = ControlSize::Compact});
    const PasswordField password({.variant = TextFieldVariant::Outlined,
                                   .size = ButtonSize::Compact});
    EXPECT_EQ(number.props().text_field_variant,password.props().text_field_variant);
    EXPECT_EQ(number.props().button_size,password.props().button_size);
}

TEST(ApiConventions, DesignatedInitializerOrderRemainsStable)
{
    const TextField input({.value = "draft",.placeholder = "Name",.key = "name",
                            .read_only = true,.variant = InputVariant::Filled,
                            .size = ControlSize::Compact});
    EXPECT_EQ(input.props().text,"draft");
    EXPECT_EQ(input.props().placeholder,"Name");
    EXPECT_EQ(input.props().key,"name");
    EXPECT_EQ(input.props().read_only,true);
}

TEST(ApiConventions, ExistingCallbackNamesRetainTheirContracts)
{
    int clicks = 0;
    std::string changed;
    const Button button({.text = "Save",.on_click = [&] {++clicks;},
                         .size = ControlSize::Medium});
    const TextField field({.on_change = [&](std::string value) {
        changed = std::move(value);
    },.size = ButtonSize::Medium});
    EXPECT_EQ(static_cast<bool>(button.props().on_click),true);
    EXPECT_EQ(static_cast<bool>(field.props().on_change),true);
    button.props().on_click();
    field.props().on_change("new value");
    EXPECT_EQ(clicks,1);
    EXPECT_EQ(changed,"new value");
}
