/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#ifndef TERION_COMPONENTS_AUTOCOMPLETE_H
#define TERION_COMPONENTS_AUTOCOMPLETE_H

#include <terion/component.h>

namespace terion {

class Autocomplete : public Component
{
public:
    struct Props
    {
        std::vector<SelectOption> options = {};
        std::string value = {};
        std::optional<std::string> default_value = std::nullopt;
        std::string query = {};
        std::string placeholder = {};
        std::string key = {};
        bool disabled = false;
        LayoutStyle layout = {};
        std::function<void(std::string)> on_change = {};
        std::function<void(std::string)> on_input_change = {};
        ButtonSize size = ButtonSize::Medium;
        std::function<void(ComponentEvent&)> on_event_capture = {};
        std::function<void(ComponentEvent&)> on_event = {};
    }; /* End of struct Props */

    explicit Autocomplete(Props props) :
        Component({.type = Type::Autocomplete,.key = std::move(props.key),
            .text = std::move(props.value),.layout = props.layout,
            .on_event_capture = std::move(props.on_event_capture),
            .on_event = std::move(props.on_event),.on_change = std::move(props.on_change),
            .placeholder = std::move(props.placeholder),.disabled = props.disabled,
            .button_size = props.size,.select_options = std::move(props.options),
            .autocomplete_query = std::move(props.query),
            .autocomplete_on_input_change = std::move(props.on_input_change),.form_default_value = props.default_value.value_or(props.value)})
    {}
}; /* End of class Autocomplete */

} /* End of namespace terion */

#endif /* TERION_COMPONENTS_AUTOCOMPLETE_H */
