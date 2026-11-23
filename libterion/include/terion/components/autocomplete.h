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
        std::string id = {};
        bool disabled = false;
        LayoutStyle layout = {};
        std::function<void(std::string)> on_change = {};
        std::function<void(std::string)> on_input_change = {};
        ControlSize size = ControlSize::Medium;
        std::function<void(ComponentEvent&)> on_event_capture = {};
        std::function<void(ComponentEvent&)> on_event = {};
    }; /* End of struct Props */

    explicit Autocomplete(Props props) :
        Component({.type = Type::Autocomplete, .key = std::move(props.key), .id = std::move(props.id), .layout = props.layout, .on_event_capture = std::move(props.on_event_capture), .on_event = std::move(props.on_event), .disabled = props.disabled, .text = Component::TextData{.text = props.default_value.value_or(props.value), .placeholder = std::move(props.placeholder)},
            .input = Component::InputData{.control_size = props.size, .form_default_value = props.default_value.value_or(props.value), .uncontrolled_value = props.default_value.has_value()},
            .action = Component::ActionData{.on_change = props.on_change ? std::move(props.on_change) :
                (props.default_value ? std::function<void(std::string)>{[](std::string) {}} :
                 std::function<void(std::string)>{})},
            .choice = Component::ChoiceData{.select_options = std::move(props.options), .autocomplete_query = std::move(props.query), .autocomplete_on_input_change = std::move(props.on_input_change)}})
    {}
}; /* End of class Autocomplete */

} /* End of namespace terion */

#endif /* TERION_COMPONENTS_AUTOCOMPLETE_H */
