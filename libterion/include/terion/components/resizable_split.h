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
        int minFirst = 3;
        int minSecond = 3;
        std::string key = {};
        LayoutStyle layout = {};
        std::function<void(int)> onChange = {};
        std::function<void(ComponentEvent&)> onEventCapture = {};
        std::function<void(ComponentEvent&)> onEvent = {};
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
            .on_event_capture = std::move(props.onEventCapture),
            .on_event = std::move(props.onEvent),.split_direction = props.direction,
            .split_position = props.position,.split_min_first = props.minFirst,
            .split_min_second = props.minSecond,.split_on_change = std::move(props.onChange)};
    }
}; /* End of class ResizableSplit */

} /* End of namespace terion */

#endif /* TERION_COMPONENTS_RESIZABLE_SPLIT_H */
