/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <terion/terion.h>
#include <gtest/gtest.h>

using namespace terion;

namespace {

Component buildForm(std::function<void(const FormValues&)> on_submit = {},
    std::function<void()> on_reset = {})
{
    return Form({.children = {
        FormField({.name = "host",.label = "Host",.required = true,
            .control = TextField({.default_value = "localhost"})}),
        FormField({.name = "port",.label = "Port",.required = true,
            .control = NumberField({.value = 8080,.min_value = 1,.max_value = 65535,
                .validator = [](double number) -> std::optional<std::string> {
                    if (number < 1024) return "Port must be at least 1024";
                    return std::nullopt;
                }})}),
        FormField({.name = "enabled",.control = Checkbox({.default_checked = true})})
    },.key = "form",.on_submit = std::move(on_submit),.on_reset = std::move(on_reset)});
}

RenderNode* fieldControl(Reconciler& reconciler,std::size_t index,std::size_t control_index = 1)
{
    return reconciler.root()->children().at(index)->children().at(control_index).get();
}

} /* End of anonymous namespace */

TEST(Form, InitializesTypedValuesWithoutPublicState)
{
    FormValues submitted;
    Reconciler reconciler;
    reconciler.update(buildForm([&](const FormValues& values) { submitted = values; }));
    const auto& children = reconciler.root()->children();
    ASSERT_EQ(children.size(),4u);
    EXPECT_EQ(fieldControl(reconciler,0)->text(),"localhost");
    EXPECT_EQ(fieldControl(reconciler,1)->text(),"8080");
    EXPECT_TRUE(children[2]->children()[0]->checkable());
    children[3]->children()[0]->onClick()();
    EXPECT_EQ(submitted.get<std::string>("host"),"localhost");
    EXPECT_DOUBLE_EQ(submitted.get<double>("port"),8080);
    EXPECT_EQ(submitted.get<int>("port"),8080);
    EXPECT_TRUE(submitted.get<bool>("enabled"));
}

TEST(Form, NumberEditingPreservesPartialInputAndValidationBlocksSubmit)
{
    int submits = 0;
    Reconciler reconciler;
    const auto build = [&] { return buildForm([&](const FormValues&) { ++submits; }); };
    reconciler.update(build());
    auto* number = fieldControl(reconciler,1);
    EXPECT_TRUE(number->handleTextInput(Event{.key = Key::Character,.character = U'a',.ctrl = true}));
    EXPECT_TRUE(number->handleTextInput(Event{.key = Key::Character,.character = U'-'}));
    reconciler.update(build());
    number = fieldControl(reconciler,1);
    // The raw draft remains visible even while it is not a complete number.
    EXPECT_FALSE(number->text().empty());
    const auto& actions = reconciler.root()->children().back()->children();
    actions[0]->onClick()();
    EXPECT_EQ(submits,0);
}

TEST(Form, ResetRestoresDefaults)
{
    int reset_count = 0;
    FormValues submitted;
    Reconciler reconciler;
    const auto build = [&] { return buildForm([&](const FormValues& values) { submitted = values; },
        [&] { ++reset_count; }); };
    reconciler.update(build());
    auto* host = fieldControl(reconciler,0);
    EXPECT_TRUE(host->handleTextInput(Event{.key = Key::Character,.character = U'X'}));
    reconciler.update(build());
    EXPECT_NE(fieldControl(reconciler,0)->text(),"localhost");
    reconciler.root()->children().back()->children()[1]->onClick()();
    reconciler.update(build());
    EXPECT_EQ(fieldControl(reconciler,0)->text(),"localhost");
    EXPECT_EQ(reset_count,1);
    reconciler.root()->children().back()->children()[0]->onClick()();
    EXPECT_EQ(submitted.get<std::string>("host"),"localhost");
}

TEST(Form, RejectsDuplicateFieldNames)
{
    const Component invalid = Form({.children = {
        FormField({.name = "same",.control = TextField({})}),
        FormField({.name = "same",.control = TextField({})})
    }});
    Reconciler reconciler;
    EXPECT_THROW(reconciler.update(invalid),std::invalid_argument);
}

TEST(FormValues, TypedAccessReportsMismatchedType)
{
    FormValues values;
    values["name"] = std::string("test");
    values["number"] = 3.14;
    EXPECT_EQ(values.get<std::string>("name"),"test");
    EXPECT_DOUBLE_EQ(values.get<double>("number"),3.14);
    EXPECT_THROW((void)values.get<bool>("name"),std::bad_variant_access);
    EXPECT_THROW((void)values.get<double>("missing"),std::out_of_range);
    EXPECT_THROW((void)values.get<int>("number"),std::out_of_range);
}
