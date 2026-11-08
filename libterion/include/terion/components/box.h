/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#ifndef TERION_COMPONENTS_BOX_H
#define TERION_COMPONENTS_BOX_H

#include <terion/component.h>

namespace terion {

class Box : public Component
{
public:
    struct Props
    {
        std::vector<Component> children = {};
        std::string key = {};
        LayoutStyle layout = {};
        std::optional<BorderStyle> border = std::nullopt;
        std::optional<Color> background = std::nullopt;
        std::function<void(ComponentEvent&)> onEventCapture = {};
        std::function<void(ComponentEvent&)> onEvent = {};
    }; /* End of struct Props */

    explicit Box(Props props) :
        Component({.type = Type::Box,.key = std::move(props.key),
            .children = std::move(props.children),.layout = props.layout,
            .on_event_capture = std::move(props.onEventCapture),.on_event = std::move(props.onEvent),
            .border = props.border,.background = props.background})
    {}
}; /* End of class Box */

} /* End of namespace terion */

#endif /* TERION_COMPONENTS_BOX_H */
