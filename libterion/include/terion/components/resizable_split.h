/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#ifndef TERION_COMPONENTS_RESIZABLE_SPLIT_H
#define TERION_COMPONENTS_RESIZABLE_SPLIT_H

#include <stdexcept>
#include <terion/component.h>

namespace terion {

class ResizableSplit : public Component
{
public:
    struct Props
    {
        std::vector<Component> children = {};
        SplitDirection direction = SplitDirection::Horizontal;
        int position = 20;
        int min_first = 3;
        int min_second = 3;
        std::string key = {};
        LayoutStyle layout = {};
        std::function<void(int)> on_change = {};
        std::function<void(ComponentEvent&)> on_event_capture = {};
        std::function<void(ComponentEvent&)> on_event = {};
    }; /* End of struct Props */

    explicit ResizableSplit(Props props) :
        Component(makeProps(std::move(props)))
    {}

private:
    static Component::Props makeProps(Props props)
    {
        if (props.children.size() != 2)
            throw std::invalid_argument("ResizableSplit requires exactly two children");
        return {.type = Type::ResizableSplit,.key = std::move(props.key),
            .children = std::move(props.children),.layout = props.layout,
            .on_event_capture = std::move(props.on_event_capture),
            .on_event = std::move(props.on_event),.split_direction = props.direction,
            .split_position = props.position,.split_min_first = props.min_first,
            .split_min_second = props.min_second,.split_on_change = std::move(props.on_change)};
    }
}; /* End of class ResizableSplit */

} /* End of namespace terion */

#endif /* TERION_COMPONENTS_RESIZABLE_SPLIT_H */
