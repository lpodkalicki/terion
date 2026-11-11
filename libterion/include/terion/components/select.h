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
        std::string placeholder = {};
        std::string key = {};
        bool disabled = false;
        LayoutStyle layout = {};
        std::function<void(std::string)> on_change = {};
        ButtonSize size = ButtonSize::Medium;
        std::function<void(ComponentEvent&)> on_event_capture = {};
        std::function<void(ComponentEvent&)> on_event = {};
    }; /* End of struct Props */

    explicit Select(Props props) :
        Component({.type = Type::Select,.key = std::move(props.key),.text = std::move(props.value),
            .layout = props.layout,.on_event_capture = std::move(props.on_event_capture),
            .on_event = std::move(props.on_event),.on_change = std::move(props.on_change),
            .placeholder = std::move(props.placeholder),.disabled = props.disabled,
            .button_size = props.size,.select_options = std::move(props.options)})
    {}
}; /* End of class Select */

} /* End of namespace terion */

#endif /* TERION_COMPONENTS_SELECT_H */
