/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#ifndef TERION_COMPONENTS_ROW_H
#define TERION_COMPONENTS_ROW_H

#include <terion/component.h>

namespace terion {

class Row : public Component
{
public:
    struct Props
    {
        std::vector<Component> children = {};
        std::string key = {};
        LayoutStyle layout = {};
    }; /* End of struct Props */

    explicit Row(Props props) :
        Component(Component::row(std::move(props.children),std::move(props.key),props.layout))
    {}
}; /* End of class Row */

} /* End of namespace terion */

#endif /* TERION_COMPONENTS_ROW_H */
