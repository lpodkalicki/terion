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
        std::string id = {};
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

    explicit Checkbox(Props props) :
        Component({.type = Type::Checkbox, .key = std::move(props.key), .id = std::move(props.id), .layout = props.layout, .on_event_capture = std::move(props.on_event_capture), .on_event = std::move(props.on_event), .disabled = props.disabled, .text = Component::TextData{.text = std::move(props.label), .color_variant = props.color},
            .input = Component::InputData{.form_default_value = props.default_checked.value_or(props.checked), .uncontrolled_value = props.default_checked.has_value()},
            .action = Component::ActionData{.on_toggle = props.on_change ? std::move(props.on_change) :
                (props.default_checked ? std::function<void(bool)>{[](bool) {}} :
                 std::function<void(bool)>{}), .checkbox_variant = props.variant, .checked = props.default_checked.value_or(props.checked)}})
    {}
}; /* End of class Checkbox */

} /* End of namespace terion */

#endif /* TERION_COMPONENTS_CHECKBOX_H */
