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
        std::function<void(std::string)> onChange = {};
        ButtonSize size = ButtonSize::Medium;
        std::function<void(ComponentEvent&)> onEventCapture = {};
        std::function<void(ComponentEvent&)> onEvent = {};
    }; /* End of struct Props */

    explicit Select(Props props) :
        Component({.type = Type::Select,.key = std::move(props.key),.text = std::move(props.value),
            .layout = props.layout,.on_event_capture = std::move(props.onEventCapture),
            .on_event = std::move(props.onEvent),.on_change = std::move(props.onChange),
            .placeholder = std::move(props.placeholder),.disabled = props.disabled,
            .button_size = props.size,.select_options = std::move(props.options)})
    {}
}; /* End of class Select */

} /* End of namespace terion */

#endif /* TERION_COMPONENTS_SELECT_H */
