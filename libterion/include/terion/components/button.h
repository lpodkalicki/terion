/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#ifndef TERION_COMPONENTS_BUTTON_H
#define TERION_COMPONENTS_BUTTON_H

#include <terion/component.h>

namespace terion {

class Button : public Component
{
public:
    struct Props
    {
        std::string text = {};
        std::string key = {};
        ButtonVariant variant = ButtonVariant::Outlined;
        ColorVariant color = ColorVariant::Primary;
        bool disabled = false;
        std::function<void()> on_click = {};
        TextStyle style = {};
        LayoutStyle layout = {};
        ButtonSize size = ButtonSize::Medium;
        std::function<void(ComponentEvent&)> on_event_capture = {};
        std::function<void(ComponentEvent&)> on_event = {};
    }; /* End of struct Props */

    explicit Button(Props props) :
        Component({.type = Type::Button,.key = std::move(props.key),.text = std::move(props.text),
            .layout = props.layout,.text_style = props.style,.color_variant = props.color,
            .on_click = std::move(props.on_click),
            .on_event_capture = std::move(props.on_event_capture),.on_event = std::move(props.on_event),
            .disabled = props.disabled,
            .button_variant = props.variant,.button_size = props.size})
    {}
}; /* End of class Button */

} /* End of namespace terion */

#endif /* TERION_COMPONENTS_BUTTON_H */
