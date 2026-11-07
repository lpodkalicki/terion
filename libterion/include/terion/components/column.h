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
    }; /* End of struct Props */

    explicit Column(Props props) :
        Component(Component::column(std::move(props.children),std::move(props.key),props.layout))
    {}
}; /* End of class Column */

} /* End of namespace terion */

#endif /* TERION_COMPONENTS_COLUMN_H */
