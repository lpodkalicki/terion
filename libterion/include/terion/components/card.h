/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#ifndef TERION_COMPONENTS_CARD_H
#define TERION_COMPONENTS_CARD_H

#include <terion/components/paper.h>
#include <terion/components/typography.h>

namespace terion {

class Card : public Component
{
public:
    struct Props
    {
        std::string title = {};
        std::vector<Component> children = {};
        std::string key = {};
        LayoutStyle layout = {.padding = 1,.gap = 1};
        std::optional<BorderStyle> border = BorderStyle::Rounded;
        std::optional<Color> background = std::nullopt;
    }; /* End of struct Props */

    explicit Card(Props props) : Component(create(std::move(props))) {}

private:
    static Component create(Props props)
    {
        if (!props.title.empty())
        {
            std::vector<Component> content;
            content.reserve(props.children.size() + 1);
            content.emplace_back(Typography({.text = std::move(props.title),.variant = TypographyVariant::Subtitle}));
            for (auto& child : props.children) content.push_back(std::move(child));
            props.children = std::move(content);
        }
        return Component({.type = Type::Box,.key = std::move(props.key),
            .children = std::move(props.children),.layout = props.layout,
            .border = props.border,.background = props.background,.surface_tone = SurfaceTone::Surface});
    }
}; /* End of class Card */

} /* End of namespace terion */

#endif /* TERION_COMPONENTS_CARD_H */
