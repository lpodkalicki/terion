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
        std::string id = {};
        LayoutStyle layout = {};
        TextStyle style = {};
        std::optional<TextStyleSpec> style_spec = std::nullopt;
    }; /* End of struct Props */

    explicit Divider(Props props) : Component({.type = Type::Divider,.key = std::move(props.key),.id = std::move(props.id),
        .layout = props.layout,.text_style = props.style,
        .text_style_spec = std::move(props.style_spec)})
    {}
}; /* End of class Divider */

} /* End of namespace terion */

#endif /* TERION_COMPONENTS_DIVIDER_H */
