/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#ifndef TERION_COMPONENTS_SELECT_H
#define TERION_COMPONENTS_SELECT_H

#include <terion/component.h>

namespace terion {

class Select : public Component
{
public:
    struct Props
    {
        std::vector<SelectOption> options = {};
        std::string value = {};
        std::optional<std::string> default_value = std::nullopt;
        std::string placeholder = {};
        std::string key = {};
        std::string id = {};
        bool disabled = false;
        LayoutStyle layout = {};
        std::function<void(std::string)> on_change = {};
        ControlSize size = ControlSize::Medium;
        std::function<void(ComponentEvent&)> on_event_capture = {};
        std::function<void(ComponentEvent&)> on_event = {};
    }; /* End of struct Props */

    explicit Select(Props props) :
        Component({.type = Type::Select,.key = std::move(props.key),.id = std::move(props.id),.text = props.default_value.value_or(props.value),
            .layout = props.layout,.on_event_capture = std::move(props.on_event_capture),
            .on_event = std::move(props.on_event),.on_change = props.on_change ? std::move(props.on_change) :
                (props.default_value ? std::function<void(std::string)>{[](std::string) {}} :
                 std::function<void(std::string)>{}),
            .placeholder = std::move(props.placeholder),.disabled = props.disabled,
            .control_size = props.size,.select_options = std::move(props.options),.form_default_value = props.default_value.value_or(props.value),.uncontrolled_value = props.default_value.has_value()})
    {}
}; /* End of class Select */

} /* End of namespace terion */

#endif /* TERION_COMPONENTS_SELECT_H */
