/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#ifndef TERION_COMPONENTS_FORM_H
#define TERION_COMPONENTS_FORM_H

#include <cmath>
#include <memory>
#include <stdexcept>
#include <utility>
#include <terion/hooks.h>
#include <terion/form_state.h>
#include <terion/components/button.h>
#include <terion/components/column.h>
#include <terion/components/row.h>
#include <terion/components/typography.h>

namespace terion {

namespace detail {

inline bool isFormInput(Component::Type type)
{
    return type == Component::Type::TextField || type == Component::Type::TextArea ||
        type == Component::Type::NumberField || type == Component::Type::PasswordField ||
        type == Component::Type::Select || type == Component::Type::Autocomplete ||
        type == Component::Type::RadioGroup || type == Component::Type::Checkbox ||
        type == Component::Type::Switch;
}

inline FormValue initialFormValue(const Component::Props& control)
{
    if (control.form_default_value) return *control.form_default_value;
    if (control.type == Component::Type::Checkbox || control.type == Component::Type::Switch)
        return control.checked;
    if (control.type == Component::Type::NumberField)
    {
        if (control.text.empty()) return std::monostate{};
        const auto number = parseNumber(control.text);
        return number ? FormValue(*number) : FormValue(std::monostate{});
    }
    return control.text;
}

inline void collectFormFields(const std::vector<Component>& children,
    std::vector<FormFieldDefinition>& fields)
{
    for (const auto& child : children)
    {
        const auto& marker = child.props();
        if (marker.type != Component::Type::FormFieldMarker)
        {
            collectFormFields(marker.children,fields);
            continue;
        }
        if (marker.children.size() != 1 || !isFormInput(marker.children.front().props().type))
            throw std::invalid_argument("FormField requires exactly one supported input control");
        const auto& control = marker.children.front().props();
        fields.push_back({.name = marker.form_field_name,
            .initial_value = initialFormValue(control),.required = marker.form_field_required,
            .validator = control.form_validator});
    }
}

inline Component bindFormField(Component component,const std::shared_ptr<FormState>& state)
{
    const auto& marker = component.props();
    const auto name = marker.form_field_name;
    auto control = marker.children.front().props();
    const auto current = state->value(name);
    if (control.type == Component::Type::Checkbox || control.type == Component::Type::Switch)
    {
        control.checked = std::get<bool>(current);
        const auto previous = std::move(control.on_toggle);
        control.on_toggle = [state,name,previous](bool checked) {
            state->setValue(name,checked);
            if (previous) previous(checked);
        };
    }
    else
    {
        control.text = control.type == Component::Type::NumberField ?
            state->displayValue(name) : toText(current);
        const auto previous = std::move(control.on_change);
        const auto number = control.type == Component::Type::NumberField;
        control.on_change = [state,name,previous,number](std::string value) {
            if (number)
            {
                state->setNumberText(name,value);
            }
            else state->setValue(name,value);
            if (previous) previous(std::move(value));
        };
    }

    std::vector<Component> children;
    if (!marker.form_field_label.empty())
        children.emplace_back(Typography({.text = marker.form_field_label}));
    children.emplace_back(Component(std::move(control)));
    const auto error = state->error(name);
    if (!error.empty())
        children.emplace_back(Typography({.text = error,.color = ColorVariant::Error}));
    return Column({.children = std::move(children),
        .key = marker.key.empty() ? name : marker.key,.layout = marker.layout});
}

inline Component bindFormTree(const Component& component,const std::shared_ptr<FormState>& state)
{
    if (component.props().type == Component::Type::FormFieldMarker)
        return bindFormField(component,state);
    auto props = component.props();
    for (auto& child : props.children) child = bindFormTree(child,state);
    return Component(std::move(props));
}

} /* End of namespace detail */

class Form : public Component
{
public:
    struct Props
    {
        std::vector<Component> children = {};
        std::string key = {};
        LayoutStyle layout = {};
        std::string submit_text = "Submit";
        std::string reset_text = "Reset";
        bool show_actions = true;
        std::function<void(const FormValues&)> on_submit = {};
        std::function<void()> on_reset = {};
    }; /* End of struct Props */

    explicit Form(Props props) : Component(create(std::move(props))) {}

private:
    static Component create(Props props)
    {
        const auto key = props.key;
        auto factory = component([props = std::move(props)]() -> Component {
            std::vector<detail::FormFieldDefinition> fields;
            detail::collectFormFields(props.children,fields);
            auto state = useRef(detail::FormState(std::move(fields)));
            std::vector<Component> children;
            children.reserve(props.children.size() + (props.show_actions ? 1u : 0u));
            for (const auto& child : props.children)
                children.emplace_back(detail::bindFormTree(child,state));
            if (props.show_actions)
            {
                std::vector<Component> actions;
                actions.emplace_back(Button({.text = props.submit_text,.key = "form-submit",
                    .on_click = [state,on_submit = props.on_submit] { (void)state->submit(on_submit); }}));
                actions.emplace_back(Button({.text = props.reset_text,.key = "form-reset",
                    .on_click = [state,on_reset = props.on_reset] {
                        state->reset();
                        if (on_reset) on_reset();
                    }}));
                children.emplace_back(Row({.children = std::move(actions),.key = "form-actions",
                    .layout = {.gap = 1}}));
            }
            return Column({.children = std::move(children),.layout = props.layout});
        });
        return factory().keyed(key);
    }
}; /* End of class Form */

} /* End of namespace terion */

#endif /* TERION_COMPONENTS_FORM_H */
