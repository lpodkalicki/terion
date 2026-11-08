/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#ifndef TERION_COMPONENTS_DROPDOWN_MENU_H
#define TERION_COMPONENTS_DROPDOWN_MENU_H

#include <terion/component.h>

namespace terion {

class DropdownMenu : public Component
{
public:
    struct Props
    {
        std::string label = "Menu";
        std::vector<SelectOption> options = {};
        std::string key = {};
        bool disabled = false;
        ButtonVariant variant = ButtonVariant::Text;
        LayoutStyle layout = {};
        std::function<void(std::string)> onSelect = {};
        ButtonSize size = ButtonSize::Medium;
        std::function<void(ComponentEvent&)> onEventCapture = {};
        std::function<void(ComponentEvent&)> onEvent = {};
    }; /* End of struct Props */

    explicit DropdownMenu(Props props) :
        Component({.type = Type::DropdownMenu,.key = std::move(props.key),
            .text = std::move(props.label),.layout = props.layout,
            .on_event_capture = std::move(props.onEventCapture),.on_event = std::move(props.onEvent),
            .on_change = std::move(props.onSelect),.disabled = props.disabled,
            .button_variant = props.variant,.button_size = props.size,
            .select_options = std::move(props.options)})
    {}
}; /* End of class DropdownMenu */

} /* End of namespace terion */

#endif /* TERION_COMPONENTS_DROPDOWN_MENU_H */
