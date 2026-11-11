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
        std::function<void(std::string)> on_select = {};
        ButtonSize size = ButtonSize::Medium;
        std::function<void(ComponentEvent&)> on_event_capture = {};
        std::function<void(ComponentEvent&)> on_event = {};
    }; /* End of struct Props */

    explicit DropdownMenu(Props props) :
        Component({.type = Type::DropdownMenu,.key = std::move(props.key),
            .text = std::move(props.label),.layout = props.layout,
            .on_event_capture = std::move(props.on_event_capture),.on_event = std::move(props.on_event),
            .on_change = std::move(props.on_select),.disabled = props.disabled,
            .button_variant = props.variant,.button_size = props.size,
            .select_options = std::move(props.options)})
    {}
}; /* End of class DropdownMenu */

} /* End of namespace terion */

#endif /* TERION_COMPONENTS_DROPDOWN_MENU_H */
