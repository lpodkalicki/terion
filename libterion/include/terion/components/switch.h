/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#ifndef TERION_COMPONENTS_SWITCH_H
#define TERION_COMPONENTS_SWITCH_H

#include <terion/component.h>

namespace terion {

class Switch : public Component
{
public:
    struct Props
    {
        std::string label = {};
        std::string key = {};
        bool checked = false;
        std::optional<bool> default_checked = std::nullopt;
        CheckboxVariant variant = CheckboxVariant::Default;
        ColorVariant color = ColorVariant::Primary;
        bool disabled = false;
        std::function<void(bool)> on_change = {};
        LayoutStyle layout = {};
        std::function<void(ComponentEvent&)> on_event_capture = {};
        std::function<void(ComponentEvent&)> on_event = {};
    }; /* End of struct Props */

    explicit Switch(Props props) :
        Component({.type = Type::Switch,.key = std::move(props.key),.text = std::move(props.label),
            .layout = props.layout,.color_variant = props.color,.on_toggle = std::move(props.on_change),
            .on_event_capture = std::move(props.on_event_capture),.on_event = std::move(props.on_event),
            .disabled = props.disabled,.checkbox_variant = props.variant,.checked = props.checked,
            .form_default_value = props.default_checked.value_or(props.checked)})
    {}
}; /* End of class Switch */

} /* End of namespace terion */

#endif /* TERION_COMPONENTS_SWITCH_H */
