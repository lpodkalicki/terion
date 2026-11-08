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
        bool disabled = false;
        std::function<void()> onClick = {};
        TextStyle style = {};
        LayoutStyle layout = {};
        ButtonSize size = ButtonSize::Medium;
    }; /* End of struct Props */

    explicit Button(Props props) :
        Component({.type = Type::Button,.key = std::move(props.key),.text = std::move(props.text),
            .layout = props.layout,.text_style = props.style,.on_click = std::move(props.onClick),.disabled = props.disabled,
            .button_variant = props.variant,.button_size = props.size})
    {}
}; /* End of class Button */

} /* End of namespace terion */

#endif /* TERION_COMPONENTS_BUTTON_H */
