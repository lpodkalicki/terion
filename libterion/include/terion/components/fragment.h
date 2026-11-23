/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#ifndef TERION_COMPONENTS_FRAGMENT_H
#define TERION_COMPONENTS_FRAGMENT_H

#include <terion/component.h>

namespace terion {

class Fragment : public Component
{
public:
    struct Props
    {
        std::vector<Component> children = {};
        std::string key = {};
        std::string id = {};
    };

    explicit Fragment(Props props) :
        Component({.type = Type::Fragment, .key = std::move(props.key), .id = std::move(props.id), .children = std::move(props.children)})
    {}
};

} /* End of namespace terion */

#endif /* TERION_COMPONENTS_FRAGMENT_H */
