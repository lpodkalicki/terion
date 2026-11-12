/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#ifndef TERION_COMPONENTS_NUMBER_FIELD_H
#define TERION_COMPONENTS_NUMBER_FIELD_H

#include <terion/component.h>

namespace terion {

class NumberField : public Component
{
public:
    struct Props
    {
        std::string value = {};
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
    }; /* End of struct Props */

    explicit NumberField(Props props) :
        Component({.type = Type::NumberField,.key = std::move(props.key),
            .text = std::move(props.value),.layout = props.layout,
            .on_change = std::move(props.on_change),.placeholder = std::move(props.placeholder),
            .disabled = props.disabled,.read_only = props.read_only,
            .number_min = props.min_value,.number_max = props.max_value,.number_step = props.step,
            .button_size = props.size,.text_field_variant = props.variant})
    {}
}; /* End of class NumberField */

} /* End of namespace terion */

#endif /* TERION_COMPONENTS_NUMBER_FIELD_H */
