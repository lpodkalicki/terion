/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#ifndef TERION_COMPONENTS_PAPER_H
#define TERION_COMPONENTS_PAPER_H

#include <terion/component.h>

namespace terion {

enum class PaperVariant
{
    Filled,
    Outlined
}; /* End of enum PaperVariant */

class Paper : public Component
{
public:
    struct Props
    {
        std::vector<Component> children = {};
        std::string key = {};
        LayoutStyle layout = {.padding = 1};
        PaperVariant variant = PaperVariant::Filled;
        std::optional<BorderStyle> border = std::nullopt;
        std::optional<Color> background = std::nullopt;
    }; /* End of struct Props */

    explicit Paper(Props props) :
        Component({.type = Type::Box,.key = std::move(props.key),
            .children = std::move(props.children),.layout = props.layout,
            .border = props.border ? props.border :
                (props.variant == PaperVariant::Outlined ? std::optional<BorderStyle>{BorderStyle::Single} : std::nullopt),
            .background = props.background,.surface_tone = SurfaceTone::Surface})
    {}
}; /* End of class Paper */

} /* End of namespace terion */

#endif /* TERION_COMPONENTS_PAPER_H */
