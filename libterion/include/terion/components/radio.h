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
        std::function<void(bool)> onChange = {};
        LayoutStyle layout = {};
        std::function<void(ComponentEvent&)> onEventCapture = {};
        std::function<void(ComponentEvent&)> onEvent = {};
    }; /* End of struct Props */

    explicit Radio(Props props) :
        Component({.type = Type::Radio,.key = std::move(props.key),.text = std::move(props.label),
            .layout = props.layout,.color_variant = props.color,.on_toggle = std::move(props.onChange),
            .on_event_capture = std::move(props.onEventCapture),.on_event = std::move(props.onEvent),
            .disabled = props.disabled,.checked = props.checked})
    {}
}; /* End of class Radio */

} /* End of namespace terion */

#endif /* TERION_COMPONENTS_RADIO_H */
