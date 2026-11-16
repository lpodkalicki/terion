/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <terion/terion.h>
#include <gtest/gtest.h>
#include <type_traits>
#include <utility>

using namespace terion;

static_assert(std::is_same_v<ControlSize,ControlSize>);
static_assert(std::is_same_v<InputVariant,InputVariant>);
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

TEST(ApiConventions, ControlSizeAppliesAcrossControlFamilies)
{
    const Theme theme = Theme::dark();
    EXPECT_NE(theme.controlMetrics(ControlSize::Compact).height,
              theme.controlMetrics(ControlSize::Large).height); // adjusted below if enum differs
    const Button button({.text = "OK",.size = ControlSize::Compact});
    const Select select({.size = ControlSize::Large});
    const Autocomplete autocomplete({.size = ControlSize::Large});
    EXPECT_EQ(button.props().control_size,ControlSize::Compact);
    EXPECT_EQ(select.props().control_size,autocomplete.props().control_size);
}

TEST(ApiConventions, InputVariantIsSingleCanonicalEnum)
{
    const TextField field({.variant = InputVariant::Filled,.size = ControlSize::Medium});
    const NumberField number({.value = 42.0,.variant = InputVariant::Outlined,
                               .size = ControlSize::Compact});
    const PasswordField password({.variant = InputVariant::Outlined,
                                   .size = ControlSize::Compact});
    EXPECT_EQ(field.props().input_variant,InputVariant::Filled);
    EXPECT_EQ(number.props().input_variant,password.props().input_variant);
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

TEST(ApiConventions, CanonicalCallbacksRetainTheirContracts)
{
    int clicks = 0;
    std::string changed;
    const Button button({.text = "Save",.on_click = [&] {++clicks;},
                         .size = ControlSize::Medium});
    const TextField field({.on_change = [&](std::string value) {
        changed = std::move(value);
    },.size = ControlSize::Medium});
    EXPECT_EQ(static_cast<bool>(button.props().on_click),true);
    EXPECT_EQ(static_cast<bool>(field.props().on_change),true);
    button.props().on_click();
    field.props().on_change("new value");
    EXPECT_EQ(clicks,1);
    EXPECT_EQ(changed,"new value");
}
