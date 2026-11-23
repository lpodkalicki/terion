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
        std::string id = {};
        LayoutStyle layout = {};
        int columns = 2;
        int min_column_width = 0; // 0: fixed; >0: collapse columns on narrow screens.
    }; /* End of struct Props */

    explicit Grid(Props props) : Component(create(std::move(props))) {}

private:
    static Component create(Props props)
    {
        if (props.min_column_width > 0)
            props.layout.grid_min_column_width = props.min_column_width;
        return Component({.type = Component::Type::Grid, .key = std::move(props.key), .id = std::move(props.id), .children = std::move(props.children), .layout = props.layout, .surface = Component::SurfaceData{.columns = props.columns}});
    }
}; /* End of class Grid */

} /* End of namespace terion */

#endif /* TERION_COMPONENTS_GRID_H */
