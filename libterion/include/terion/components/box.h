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
    }; /* End of struct Props */

    explicit Box(Props props) :
        Component(Component::box(std::move(props.children),props.layout,std::move(props.key)))
    {}
}; /* End of class Box */

} /* End of namespace terion */

#endif /* TERION_COMPONENTS_BOX_H */
