/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#ifndef TERION_COMPONENTS_CHECKBOX_H
#define TERION_COMPONENTS_CHECKBOX_H

#include <terion/component.h>

namespace terion {

class Checkbox : public Component
{
public:
    struct Props
    {
        std::string label = {};
        std::string key = {};
        bool checked = false;
        CheckboxVariant variant = CheckboxVariant::Default;
        ColorVariant color = ColorVariant::Primary;
        bool disabled = false;
        std::function<void(bool)> on_change = {};
        LayoutStyle layout = {};
        std::function<void(ComponentEvent&)> on_event_capture = {};
        std::function<void(ComponentEvent&)> on_event = {};
    }; /* End of struct Props */

    explicit Checkbox(Props props) :
        Component({.type = Type::Checkbox,.key = std::move(props.key),.text = std::move(props.label),
            .layout = props.layout,.color_variant = props.color,.on_toggle = std::move(props.on_change),
            .on_event_capture = std::move(props.on_event_capture),.on_event = std::move(props.on_event),
            .disabled = props.disabled,.checkbox_variant = props.variant,.checked = props.checked})
    {}
}; /* End of class Checkbox */

} /* End of namespace terion */

#endif /* TERION_COMPONENTS_CHECKBOX_H */
