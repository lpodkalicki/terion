/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#ifndef TERION_COMPONENTS_PASSWORD_FIELD_H
#define TERION_COMPONENTS_PASSWORD_FIELD_H

#include <terion/component.h>

namespace terion {

class PasswordField : public Component
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
        bool reveal = false;
        char32_t mask = U'•';
        LayoutStyle layout = {};
        TextFieldVariant variant = TextFieldVariant::Outlined;
        ButtonSize size = ButtonSize::Medium;
        std::function<void(std::string)> on_change = {};
        std::function<std::optional<std::string>(const std::string&)> validator = {};
    }; /* End of struct Props */

    explicit PasswordField(Props props) :
        Component({.type = Type::PasswordField,.key = std::move(props.key),
            .text = std::move(props.value),.layout = props.layout,
            .on_change = std::move(props.on_change),.placeholder = std::move(props.placeholder),
            .disabled = props.disabled,.read_only = props.read_only,
            .password_reveal = props.reveal,.password_mask = props.mask,
            .button_size = props.size,.text_field_variant = props.variant,.form_default_value = props.default_value.value_or(props.value),
            .form_validator = [validator = std::move(props.validator)](const FormValue& value)
                -> std::optional<std::string> {
                if (validator) return validator(std::get<std::string>(value));
                return std::nullopt;
            }})
    {}
}; /* End of class PasswordField */

} /* End of namespace terion */

#endif /* TERION_COMPONENTS_PASSWORD_FIELD_H */
