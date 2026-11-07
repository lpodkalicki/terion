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
        bool disabled = false;
        std::function<void()> onClick = {};
        TextStyle style = {};
    }; /* End of struct Props */

    explicit Button(Props props) :
        Component({.type = Type::Button,.key = std::move(props.key),.text = std::move(props.text),
            .text_style = props.style,.on_click = std::move(props.onClick),.disabled = props.disabled})
    {}
}; /* End of class Button */

} /* End of namespace terion */

#endif /* TERION_COMPONENTS_BUTTON_H */
