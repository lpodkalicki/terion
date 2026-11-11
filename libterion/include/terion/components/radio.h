/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#ifndef TERION_COMPONENTS_RADIO_H
#define TERION_COMPONENTS_RADIO_H

#include <terion/component.h>

namespace terion {

class Radio : public Component
{
public:
    struct Props
    {
        std::string label = {};
        std::string key = {};
        bool checked = false;
        ColorVariant color = ColorVariant::Primary;
        bool disabled = false;
        std::function<void(bool)> on_change = {};
        LayoutStyle layout = {};
        std::function<void(ComponentEvent&)> on_event_capture = {};
        std::function<void(ComponentEvent&)> on_event = {};
    }; /* End of struct Props */

    explicit Radio(Props props) :
        Component({.type = Type::Radio,.key = std::move(props.key),.text = std::move(props.label),
            .layout = props.layout,.color_variant = props.color,.on_toggle = std::move(props.on_change),
            .on_event_capture = std::move(props.on_event_capture),.on_event = std::move(props.on_event),
            .disabled = props.disabled,.checked = props.checked})
    {}
}; /* End of class Radio */

} /* End of namespace terion */

#endif /* TERION_COMPONENTS_RADIO_H */
