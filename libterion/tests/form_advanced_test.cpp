/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <terion/terion.h>
#include <gtest/gtest.h>

using namespace terion;

namespace {

std::vector<detail::FormFieldDefinition> definitions()
{
    return {
        {.name = "name",.initial_value = std::string("server"),.required = true},
        {.name = "confirm",.initial_value = std::string{},.required = true,
            .validator_with_values = [](const FormValue& value,const FormValues& values)
                -> std::optional<std::string> {
                if (std::get<std::string>(value) != values.get<std::string>("name"))
                    return "Names differ";
                return std::nullopt;
            }},
        {.name = "port",.initial_value = 8080.0,.required = true,
            .validator = [](const FormValue& value) -> std::optional<std::string> {
                if (std::holds_alternative<std::monostate>(value)) return "Invalid number";
                return std::nullopt;
            }}
    };
}

} /* End of anonymous namespace */

TEST(FormAdvanced, InitialErrorsAreHiddenUntilBlurOrSubmit)
{
    detail::FormState state(definitions(),FormValidationMode::OnBlur);
    EXPECT_FALSE(state.status().valid);
    EXPECT_EQ(state.error("confirm"),"This field is required");
    EXPECT_TRUE(state.visibleError("confirm").empty());
    state.setValue("confirm",std::string("bad"));
    EXPECT_TRUE(state.visibleError("confirm").empty());
    EXPECT_EQ(state.status().touched_count,0u);
    state.markTouched("confirm");
    EXPECT_EQ(state.visibleError("confirm"),"Names differ");
    EXPECT_EQ(state.status().touched_count,1u);
    EXPECT_FALSE(state.validate());
    EXPECT_TRUE(state.status().submitted);
    EXPECT_EQ(state.status().submit_count,1u);
}

TEST(FormAdvanced, CrossFieldValidationReevaluatesDependents)
{
    detail::FormState state(definitions());
    state.setValue("confirm",std::string("server"));
    EXPECT_TRUE(state.status().valid);
    state.setValue("name",std::string("other"));
    EXPECT_EQ(state.error("confirm"),"Names differ");
    EXPECT_EQ(state.firstError(),"confirm");
    state.setValue("name",std::string("server"));
    EXPECT_TRUE(state.status().valid);
}

TEST(FormAdvanced, DirtyTracksReversionAndNumberDrafts)
{
    detail::FormState state(definitions());
    EXPECT_FALSE(state.status().dirty);
    state.setValue("name",std::string("updated"));
    EXPECT_TRUE(state.status().dirty);
    EXPECT_TRUE(state.dirty("name"));
    state.setValue("name",std::string("server"));
    EXPECT_FALSE(state.dirty("name"));
    state.setNumberText("port","-");
    EXPECT_TRUE(state.dirty("port"));
    EXPECT_FALSE(state.status().valid);
    state.setNumberText("port","8080");
    EXPECT_FALSE(state.dirty("port"));
    EXPECT_EQ(state.displayValue("port"),"8080");
}

TEST(FormAdvanced, OnSubmitDelaysErrorsAndResetClearsState)
{
    detail::FormState state(definitions(),FormValidationMode::OnSubmit);
    state.setValue("confirm",std::string("not matching"));
    state.markTouched("confirm");
    EXPECT_TRUE(state.visibleError("confirm").empty());
    EXPECT_FALSE(state.validate());
    EXPECT_EQ(state.visibleError("confirm"),"Names differ");
    state.reset();
    const auto status = state.status();
    EXPECT_FALSE(status.dirty);
    EXPECT_FALSE(status.submitted);
    EXPECT_EQ(status.submit_count,0u);
    EXPECT_EQ(status.touched_count,0u);
}

TEST(FormAdvanced, OnChangeShowsOnlyChangedFieldsUntilSubmit)
{
    detail::FormState state(definitions());
    EXPECT_TRUE(state.visibleError("confirm").empty());
    state.setValue("confirm",std::string("bad"));
    EXPECT_EQ(state.visibleError("confirm"),"Names differ");
    EXPECT_TRUE(state.visibleError("name").empty());
}

TEST(FormAdvanced, SubmitGuardRestoresStateAfterException)
{
    detail::FormState state(definitions());
    state.setValue("confirm",std::string("server"));
    EXPECT_THROW((void)state.submit([](const FormValues&) {
        throw std::runtime_error("save failed");
    }),std::runtime_error);
    EXPECT_FALSE(state.status().submitting);
    EXPECT_EQ(state.status().submit_count,1u);
}

TEST(FormAdvanced, ControllerSupportsProgrammaticValidationAndReset)
{
    auto controller = std::make_shared<FormController>();
    Reconciler reconciler;
    const auto build = [&] {
        return Form({.children = {
            FormField({.name = "title",.required = true,
                .control = TextField({.default_value = "hello"})}),
            FormField({.name = "repeat",.required = true,
                .control = TextField({}),
                .validator = [](const FormValue& v,const FormValues& values)
                    -> std::optional<std::string> {
                    return std::get<std::string>(v) == values.get<std::string>("title") ?
                        std::nullopt : std::optional<std::string>("Mismatch");
                }})
        },.key = "form",.id = "form",.validation_mode = FormValidationMode::OnBlur,
            .controller = controller});
    };
    EXPECT_FALSE(controller->ready());
    EXPECT_THROW((void)controller->status(),std::logic_error);
    reconciler.update(build());
    ASSERT_TRUE(controller->ready());
    EXPECT_EQ(controller->firstError(),"repeat");
    EXPECT_FALSE(controller->status().valid);
    controller->setValue("repeat",std::string("hello"));
    EXPECT_TRUE(controller->status().valid);
    EXPECT_TRUE(controller->status().dirty);
    reconciler.update(build());
    EXPECT_EQ(reconciler.root()->children()[1]->children()[0]->text(),"hello");
    controller->reset();
    EXPECT_FALSE(controller->status().dirty);
    EXPECT_EQ(controller->value("repeat"),FormValue(std::string{}));
    reconciler.update(Component::text("Unmount"));
    reconciler.commitEffects();
    EXPECT_FALSE(controller->ready());
}

TEST(FormAdvanced, InvalidSubmitCallsFirstErrorInFieldOrder)
{
    auto controller = std::make_shared<FormController>();
    std::string first;
    Reconciler reconciler;
    reconciler.update(Form({.children = {
        FormField({.name = "a",.required = true,.control = TextField({})}),
        FormField({.name = "b",.required = true,.control = TextField({})})
    },.controller = controller,.on_invalid = [&](const std::string& name) { first = name; }}));
    reconciler.root()->children().back()->children()[0]->onClick()();
    EXPECT_EQ(first,"a");
    EXPECT_EQ(controller->status().submit_count,1u);
}

TEST(FormAdvanced, FocusBlurCallbackRunsOnTabAndNotOnReorder)
{
    FocusManager focus;
    int a_blurs = 0;
    int b_blurs = 0;
    focus.add("a",[](const Event&) { return false; },{},[&] { ++a_blurs; });
    focus.add("b",[](const Event&) { return false; },{},[&] { ++b_blurs; });
    ASSERT_TRUE(focus.dispatch(Event{.key = Key::Tab}));
    EXPECT_EQ(focus.current(),"b");
    EXPECT_EQ(a_blurs,1);
    focus.reorder({"b","a"});
    EXPECT_EQ(b_blurs,0);
    EXPECT_EQ(focus.current(),"b");
    focus.blur();
    EXPECT_EQ(b_blurs,1);
}

TEST(FormAdvanced, ApplicationFocusFindsInputBehindCompositeWrapper)
{
    Application app(std::make_unique<HeadlessTerminalBackend>(Size{100,30}));
    auto controller = std::make_shared<FormController>();
    app.render([&] { return Form({.children = {
        FormField({.name = "host",.required = true,.control = TextField({})}),
        FormField({.name = "port",.control = TextField({})})
    },.validation_mode = FormValidationMode::OnBlur,.controller = controller}); });
    app.renderOnce();
    ASSERT_TRUE(controller->ready());
    EXPECT_TRUE(app.focus("host"));
    EXPECT_TRUE(app.hasFocus("host"));
    EXPECT_TRUE(app.focus("port"));
    EXPECT_TRUE(controller->touched("host"));
    EXPECT_FALSE(controller->touched("port"));
    EXPECT_TRUE(app.focus("form-submit"));
    EXPECT_TRUE(controller->touched("port"));
}
