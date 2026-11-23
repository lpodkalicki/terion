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
        std::string id = {};
        ButtonVariant variant = ButtonVariant::Outlined;
        ColorVariant color = ColorVariant::Primary;
        bool disabled = false;
        std::function<void()> on_click = {};
        TextStyle style = {};
        LayoutStyle layout = {};
        ControlSize size = ControlSize::Medium;
        std::function<void(ComponentEvent&)> on_event_capture = {};
        std::function<void(ComponentEvent&)> on_event = {};
        std::optional<TextStyleSpec> style_spec = std::nullopt;
    }; /* End of struct Props */

    explicit Button(Props props) :
        Component({.type = Type::Button, .key = std::move(props.key), .id = std::move(props.id), .layout = props.layout, .on_event_capture = std::move(props.on_event_capture), .on_event = std::move(props.on_event), .disabled = props.disabled, .text = Component::TextData{.text = std::move(props.text), .text_style = props.style, .color_variant = props.color, .text_style_spec = std::move(props.style_spec)},
            .input = Component::InputData{.control_size = props.size},
            .action = Component::ActionData{.on_click = std::move(props.on_click), .button_variant = props.variant}})
    {}
}; /* End of class Button */

} /* End of namespace terion */

#endif /* TERION_COMPONENTS_BUTTON_H */
