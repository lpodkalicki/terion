/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#ifndef TERION_COMPONENTS_TYPOGRAPHY_H
#define TERION_COMPONENTS_TYPOGRAPHY_H

#include <terion/component.h>

namespace terion {

enum class TypographyVariant
{
    Body,
    Subtitle,
    H1,
    H2,
    H3
}; /* End of enum TypographyVariant */

class Typography : public Component
{
public:
    struct Props
    {
        std::string text = {};
        std::string key = {};
        TypographyVariant variant = TypographyVariant::Body;
        ColorVariant color = ColorVariant::Default;
        TextStyle style = {};
        std::function<void(ComponentEvent&)> on_event_capture = {};
        std::function<void(ComponentEvent&)> on_event = {};
    }; /* End of struct Props */

    explicit Typography(Props props) : Component(create(std::move(props))) {}

private:
    static Component create(Props props)
    {
        if (props.variant != TypographyVariant::Body) props.style.bold = true;
        return Component({.type = Type::Text,.key = std::move(props.key),
            .text = std::move(props.text),.text_style = props.style,.color_variant = props.color,
            .on_event_capture = std::move(props.on_event_capture),.on_event = std::move(props.on_event)});
    }
}; /* End of class Typography */

} /* End of namespace terion */

#endif /* TERION_COMPONENTS_TYPOGRAPHY_H */
