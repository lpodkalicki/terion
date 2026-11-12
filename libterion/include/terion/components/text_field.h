/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#ifndef TERION_COMPONENTS_TEXT_FIELD_H
#define TERION_COMPONENTS_TEXT_FIELD_H

#include <terion/component.h>

namespace terion {

class TextField : public Component
{
public:
    struct Props
    {
        std::string value = {};
        std::optional<std::string> default_value = std::nullopt;
        std::string placeholder = {};
        std::string key = {};
        bool disabled = false;
        bool read_only = false;
        LayoutStyle layout = {};
        std::function<void(std::string)> on_change = {};
        std::function<std::optional<std::string>(const std::string&)> validator = {};
        TextFieldVariant variant = TextFieldVariant::Outlined;
        ButtonSize size = ButtonSize::Medium;
        std::function<void(ComponentEvent&)> on_event_capture = {};
        std::function<void(ComponentEvent&)> on_event = {};
    }; /* End of struct Props */

    explicit TextField(Props props) :
        Component({.type = Type::TextField,.key = std::move(props.key),
            .text = std::move(props.value),.layout = props.layout,
            .on_event_capture = std::move(props.on_event_capture),.on_event = std::move(props.on_event),
            .on_change = std::move(props.on_change),.placeholder = std::move(props.placeholder),
            .disabled = props.disabled,.read_only = props.read_only,.button_size = props.size,.text_field_variant = props.variant,.form_default_value = props.default_value.value_or(props.value),
            .form_validator = [validator = std::move(props.validator)](const FormValue& value)
                -> std::optional<std::string> {
                if (validator) return validator(std::get<std::string>(value));
                return std::nullopt;
            }})
    {}
}; /* End of class TextField */

} /* End of namespace terion */

#endif /* TERION_COMPONENTS_TEXT_FIELD_H */
