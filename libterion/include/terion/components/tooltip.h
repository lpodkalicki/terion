/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#ifndef TERION_COMPONENTS_TOOLTIP_H
#define TERION_COMPONENTS_TOOLTIP_H

#include <terion/component.h>

namespace terion {

// A layout-transparent wrapper: hover over its child to show a tooltip.
class Tooltip : public Component
{
public:
    struct Props
    {
        Component child;
        std::string text;
        std::string key = {};
        std::string id = {};
        bool disabled = false;
    }; /* End of struct Props */

    explicit Tooltip(Props props) :
        Component({.type = Type::Tooltip, .key = std::move(props.key), .id = std::move(props.id), .children = {std::move(props.child)}, .disabled = props.disabled, .text = Component::TextData{.text = std::move(props.text)}})
    {}
}; /* End of class Tooltip */

} /* End of namespace terion */

#endif /* TERION_COMPONENTS_TOOLTIP_H */
