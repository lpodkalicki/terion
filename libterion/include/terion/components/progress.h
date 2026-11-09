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
        LayoutStyle layout = {.width = 20};
        TextStyle style = {};
        ProgressVariant variant = ProgressVariant::Blocks;
        ColorVariant color = ColorVariant::Primary;
        bool indeterminate = false;
        bool showPercentage = false;
    }; /* End of struct Props */

    explicit Progress(Props props) : Component({.type = Type::Progress,.key = std::move(props.key),
        .layout = props.layout,.text_style = props.style,.color_variant = props.color,.value = props.value,
        .progress_variant = props.variant,.progress_indeterminate = props.indeterminate,
        .progress_show_percentage = props.showPercentage})
    {}
}; /* End of class Progress */

} /* End of namespace terion */

#endif /* TERION_COMPONENTS_PROGRESS_H */
