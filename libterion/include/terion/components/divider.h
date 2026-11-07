/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#ifndef TERION_COMPONENTS_DIVIDER_H
#define TERION_COMPONENTS_DIVIDER_H

#include <terion/component.h>

namespace terion {

class Divider : public Component
{
public:
    struct Props
    {
        std::string key = {};
        LayoutStyle layout = {};
        TextStyle style = {};
    }; /* End of struct Props */

    explicit Divider(Props props) : Component({.type = Type::Divider,.key = std::move(props.key),
        .layout = props.layout,.text_style = props.style})
    {}
}; /* End of class Divider */

} /* End of namespace terion */

#endif /* TERION_COMPONENTS_DIVIDER_H */
