/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#ifndef TERION_COMPONENTS_CONTEXT_MENU_H
#define TERION_COMPONENTS_CONTEXT_MENU_H

#include <terion/component.h>

namespace terion {

// A transparent wrapper. Right-clicking anywhere in its child opens an overlay
// at the cursor; the child continues to receive ordinary mouse/keyboard input.
class ContextMenu : public Component
{
public:
    struct Props
    {
        Component child;
        std::vector<SelectOption> options = {};
        std::string key = {};
        bool disabled = false;
        std::function<void(std::string)> onSelect = {};
        std::function<void(ComponentEvent&)> onEventCapture = {};
        std::function<void(ComponentEvent&)> onEvent = {};
    }; /* End of struct Props */

    explicit ContextMenu(Props props) :
        Component({.type = Type::ContextMenu,.key = std::move(props.key),
            .children = {std::move(props.child)},
            .on_event_capture = std::move(props.onEventCapture),
            .on_event = std::move(props.onEvent),
            .on_change = std::move(props.onSelect),.disabled = props.disabled,
            .select_options = std::move(props.options)})
    {}
}; /* End of class ContextMenu */

} /* End of namespace terion */

#endif /* TERION_COMPONENTS_CONTEXT_MENU_H */
