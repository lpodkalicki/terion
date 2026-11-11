/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#ifndef TERION_COMPONENTS_RADIO_GROUP_H
#define TERION_COMPONENTS_RADIO_GROUP_H

#include <terion/component.h>

namespace terion {

class RadioGroup : public Component
{
public:
    struct Props
    {
        std::vector<RadioOption> options = {};
        std::string value = {};
        std::string key = {};
        bool disabled = false;
        ColorVariant color = ColorVariant::Primary;
        std::function<void(std::string)> on_change = {};
        LayoutStyle layout = {};
        std::function<void(ComponentEvent&)> on_event_capture = {};
        std::function<void(ComponentEvent&)> on_event = {};
    }; /* End of struct Props */

    explicit RadioGroup(Props props) :
        Component({.type = Type::RadioGroup,.key = std::move(props.key),.text = std::move(props.value),
            .layout = props.layout,.color_variant = props.color,
            .on_event_capture = std::move(props.on_event_capture),
            .on_event = std::move(props.on_event),.on_change = std::move(props.on_change),
            .disabled = props.disabled,.radio_options = std::move(props.options)})
    {}
}; /* End of class RadioGroup */

} /* End of namespace terion */

#endif /* TERION_COMPONENTS_RADIO_GROUP_H */
