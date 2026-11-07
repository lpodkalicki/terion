/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#ifndef TERION_COMPONENTS_GRID_H
#define TERION_COMPONENTS_GRID_H

#include <terion/component.h>

namespace terion {

class Grid : public Component
{
public:
    struct Props
    {
        std::vector<Component> children = {};
        std::string key = {};
        LayoutStyle layout = {};
        int columns = 2;
    }; /* End of struct Props */

    explicit Grid(Props props) :
        Component({.type = Component::Type::Grid,.key = std::move(props.key),
            .children = std::move(props.children),.layout = props.layout,
            .columns = props.columns})
    {}
}; /* End of class Grid */

} /* End of namespace terion */

#endif /* TERION_COMPONENTS_GRID_H */
