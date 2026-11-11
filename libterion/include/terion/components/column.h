/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#ifndef TERION_COMPONENTS_COLUMN_H
#define TERION_COMPONENTS_COLUMN_H

#include <terion/component.h>

namespace terion {

class Column : public Component
{
public:
    struct Props
    {
        std::vector<Component> children = {};
        std::string key = {};
        LayoutStyle layout = {};
        std::function<void(ComponentEvent&)> on_event_capture = {};
        std::function<void(ComponentEvent&)> on_event = {};
    }; /* End of struct Props */

    explicit Column(Props props) :
        Component({.type = Type::Column,.key = std::move(props.key),
            .children = std::move(props.children),.layout = props.layout,
            .on_event_capture = std::move(props.on_event_capture),.on_event = std::move(props.on_event)})
    {}
}; /* End of class Column */

} /* End of namespace terion */

#endif /* TERION_COMPONENTS_COLUMN_H */
