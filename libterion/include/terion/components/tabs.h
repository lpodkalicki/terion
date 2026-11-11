/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#ifndef TERION_COMPONENTS_TABS_H
#define TERION_COMPONENTS_TABS_H

#include <terion/component.h>

namespace terion {

class Tabs : public Component
{
public:
    struct Props
    {
        std::vector<TabOption> options = {};
        std::string value = {};
        std::string key = {};
        bool disabled = false;
        LayoutStyle layout = {};
        std::function<void(std::string)> on_change = {};
        std::function<void(ComponentEvent&)> on_event_capture = {};
        std::function<void(ComponentEvent&)> on_event = {};
    }; /* End of struct Props */

    explicit Tabs(Props props) :
        Component({.type = Type::Tabs,.key = std::move(props.key),.text = std::move(props.value),
            .layout = props.layout,.on_event_capture = std::move(props.on_event_capture),
            .on_event = std::move(props.on_event),.on_change = std::move(props.on_change),
            .disabled = props.disabled,.tab_options = std::move(props.options)})
    {}
}; /* End of class Tabs */

} /* End of namespace terion */

#endif /* TERION_COMPONENTS_TABS_H */
