/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#ifndef TERION_COMPONENTS_TEXT_AREA_H
#define TERION_COMPONENTS_TEXT_AREA_H

#include <terion/component.h>

namespace terion {

class TextArea : public Component
{
public:
    struct Props
    {
        std::string value = {};
        std::string placeholder = {};
        std::string key = {};
        bool disabled = false;
        bool read_only = false;
        int min_rows = 4;
        LayoutStyle layout = {};
        std::function<void(std::string)> on_change = {};
        std::function<void(ComponentEvent&)> on_event_capture = {};
        std::function<void(ComponentEvent&)> on_event = {};
    }; /* End of struct Props */

    explicit TextArea(Props props) :
        Component({.type = Type::TextArea,.key = std::move(props.key),
            .text = std::move(props.value),.layout = props.layout,
            .on_event_capture = std::move(props.on_event_capture),.on_event = std::move(props.on_event),
            .on_change = std::move(props.on_change),.placeholder = std::move(props.placeholder),
            .disabled = props.disabled,.read_only = props.read_only,
            .text_area_min_rows = props.min_rows})
    {}
}; /* End of class TextArea */

} /* End of namespace terion */

#endif /* TERION_COMPONENTS_TEXT_AREA_H */
