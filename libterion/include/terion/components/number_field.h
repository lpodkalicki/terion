/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#ifndef TERION_COMPONENTS_NUMBER_FIELD_H
#define TERION_COMPONENTS_NUMBER_FIELD_H

#include <terion/component.h>
#include <terion/form_state.h>

namespace terion {

class NumberField : public Component
{
public:
    struct Props
    {
        FormValue value = std::string{};
        std::optional<FormValue> default_value = std::nullopt;
        std::string placeholder = {};
        std::string key = {};
        double min_value = -1.0e100;
        double max_value = 1.0e100;
        double step = 1.0;
        bool disabled = false;
        bool read_only = false;
        LayoutStyle layout = {};
        TextFieldVariant variant = TextFieldVariant::Outlined;
        ButtonSize size = ButtonSize::Medium;
        std::function<void(std::string)> on_change = {};
        std::function<std::optional<std::string>(double)> validator = {};
    }; /* End of struct Props */

    explicit NumberField(Props props) :
        Component(create(std::move(props)))
    {}

private:
    static Component::Props create(Props props)
    {
        const FormValue selected = props.default_value.value_or(props.value);
        FormValue initial = std::monostate{};
        if (const auto* text = std::get_if<std::string>(&selected))
        {
            const auto parsed = detail::parseNumber(*text);
            if (parsed) initial = *parsed;
        }
        else if (const auto* number = std::get_if<double>(&selected)) initial = *number;
        else if (const auto* integer = std::get_if<std::int64_t>(&selected))
            initial = static_cast<double>(*integer);
        return {.type = Type::NumberField,.key = std::move(props.key),
            .text = detail::toText(props.value),.layout = props.layout,
            .on_change = std::move(props.on_change),.placeholder = std::move(props.placeholder),
            .disabled = props.disabled,.read_only = props.read_only,
            .number_min = props.min_value,.number_max = props.max_value,.number_step = props.step,
            .button_size = props.size,.text_field_variant = props.variant,
            .form_default_value = initial,
            .form_validator = [validator = std::move(props.validator),min_value = props.min_value,
                max_value = props.max_value](const FormValue& value) -> std::optional<std::string> {
                if (std::holds_alternative<std::monostate>(value)) return "Invalid number";
                double number = 0;
                if (const auto* real = std::get_if<double>(&value)) number = *real;
                else if (const auto* integer = std::get_if<std::int64_t>(&value))
                    number = static_cast<double>(*integer);
                else return "Invalid number";
                if (number < min_value || number > max_value) return "Number is out of range";
                if (validator) return validator(number);
                return std::nullopt;
            }};
    }
}; /* End of class NumberField */

} /* End of namespace terion */

#endif /* TERION_COMPONENTS_NUMBER_FIELD_H */
