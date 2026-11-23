/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#ifndef TERION_COMPONENTS_PROGRESS_H
#define TERION_COMPONENTS_PROGRESS_H

#include <terion/component.h>

namespace terion {

class Progress : public Component
{
public:
    struct Props
    {
        double value = 0.0;
        std::string key = {};
        std::string id = {};
        LayoutStyle layout = {.width = 20};
        TextStyle style = {};
        ProgressVariant variant = ProgressVariant::Blocks;
        ColorVariant color = ColorVariant::Primary;
        bool indeterminate = false;
        bool show_percentage = false;
        std::optional<TextStyleSpec> style_spec = std::nullopt;
    }; /* End of struct Props */

    explicit Progress(Props props) : Component({.type = Type::Progress, .key = std::move(props.key), .id = std::move(props.id), .layout = props.layout, .text = Component::TextData{.text_style = props.style, .color_variant = props.color, .text_style_spec = std::move(props.style_spec)},
            .action = Component::ActionData{.value = props.value},
            .progress = Component::ProgressData{.progress_variant = props.variant, .progress_indeterminate = props.indeterminate, .progress_show_percentage = props.show_percentage}})
    {}
}; /* End of class Progress */

} /* End of namespace terion */

#endif /* TERION_COMPONENTS_PROGRESS_H */
