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
        std::optional<std::string> default_value = std::nullopt;
        std::string key = {};
        std::string id = {};
        bool disabled = false;
        ColorVariant color = ColorVariant::Primary;
        std::function<void(std::string)> on_change = {};
        LayoutStyle layout = {};
        std::function<void(ComponentEvent&)> on_event_capture = {};
        std::function<void(ComponentEvent&)> on_event = {};
    }; /* End of struct Props */

    explicit RadioGroup(Props props) :
        Component({.type = Type::RadioGroup, .key = std::move(props.key), .id = std::move(props.id), .layout = props.layout, .on_event_capture = std::move(props.on_event_capture), .on_event = std::move(props.on_event), .disabled = props.disabled, .text = Component::TextData{.text = props.default_value.value_or(props.value), .color_variant = props.color},
            .input = Component::InputData{.form_default_value = props.default_value.value_or(props.value), .uncontrolled_value = props.default_value.has_value()},
            .action = Component::ActionData{.on_change = props.on_change ? std::move(props.on_change) :
                (props.default_value ? std::function<void(std::string)>{[](std::string) {}} :
                 std::function<void(std::string)>{})},
            .choice = Component::ChoiceData{.radio_options = std::move(props.options)}})
    {}
}; /* End of class RadioGroup */

} /* End of namespace terion */

#endif /* TERION_COMPONENTS_RADIO_GROUP_H */
