/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#ifndef TERION_COMPONENTS_STACK_H
#define TERION_COMPONENTS_STACK_H

#include <terion/component.h>

namespace terion {

enum class StackDirection
{
    Vertical,
    Horizontal
}; /* End of enum StackDirection */

class Stack : public Component
{
public:
    struct Props
    {
        std::vector<Component> children = {};
        std::string key = {};
        LayoutStyle layout = {};
        StackDirection direction = StackDirection::Vertical;
    }; /* End of struct Props */

    explicit Stack(Props props) :
        Component(props.direction == StackDirection::Vertical ?
            Component::column(std::move(props.children),std::move(props.key),props.layout) :
            Component::row(std::move(props.children),std::move(props.key),props.layout))
    {}
}; /* End of class Stack */

} /* End of namespace terion */

#endif /* TERION_COMPONENTS_STACK_H */
