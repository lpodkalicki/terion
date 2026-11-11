/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#ifndef TERION_COMPONENTS_FOCUS_SCOPE_H
#define TERION_COMPONENTS_FOCUS_SCOPE_H

#include <terion/component.h>

namespace terion {

class FocusScope : public Component
{
public:
    struct Props
    {
        std::vector<Component> children = {};
        std::string key = {};
        LayoutStyle layout = {};
        bool active = false;
        std::function<void(ComponentEvent&)> on_event_capture = {};
        std::function<void(ComponentEvent&)> on_event = {};
    }; /* End of struct Props */

    explicit FocusScope(Props props) :
        Component({.type = Type::FocusScope,.key = std::move(props.key),
            .children = std::move(props.children),.layout = props.layout,
            .on_event_capture = std::move(props.on_event_capture),.on_event = std::move(props.on_event),
            .focus_scope_active = props.active})
    {}
}; /* End of class FocusScope */

} /* End of namespace terion */

#endif /* TERION_COMPONENTS_FOCUS_SCOPE_H */
