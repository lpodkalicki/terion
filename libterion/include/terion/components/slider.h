/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#ifndef TERION_COMPONENTS_SLIDER_H
#define TERION_COMPONENTS_SLIDER_H

#include <functional>
#include <string>
#include <terion/component.h>

namespace terion {

class Slider : public Component
{
public:
    struct Props
    {
        double value = 0.0;
        double min = 0.0;
        double max = 100.0;
        double step = 1.0;
        std::string key = {};
        bool disabled = false;
        bool showValue = true;
        ColorVariant color = ColorVariant::Primary;
        std::function<void(double)> onChange = {};
        LayoutStyle layout = {.width = 25};
    }; /* End of struct Props */

    explicit Slider(Props props) : Component({.type = Type::Slider,.key = std::move(props.key),
        .layout = props.layout,.color_variant = props.color,.disabled = props.disabled,.value = props.value,
        .slider_min = props.min,.slider_max = props.max,.slider_step = props.step,
        .slider_show_value = props.showValue,.slider_on_change = std::move(props.onChange)})
    {}
}; /* End of class Slider */

} /* End of namespace terion */

#endif /* TERION_COMPONENTS_SLIDER_H */
