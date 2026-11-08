/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#ifndef TERION_COMPONENTS_TAB_PANEL_H
#define TERION_COMPONENTS_TAB_PANEL_H

#include <terion/component.h>

namespace terion {

class TabPanel : public Component
{
public:
    struct Props
    {
        std::vector<Component> children = {};
        std::string key = {};
        bool active = true;
        LayoutStyle layout = {};
        std::function<void(ComponentEvent&)> onEventCapture = {};
        std::function<void(ComponentEvent&)> onEvent = {};
    }; /* End of struct Props */

    explicit TabPanel(Props props) :
        Component({.type = Type::TabPanel,.key = std::move(props.key),
            .children = std::move(props.children),.layout = props.layout,
            .on_event_capture = std::move(props.onEventCapture),
            .on_event = std::move(props.onEvent),.tab_panel_active = props.active})
    {}
}; /* End of class TabPanel */

} /* End of namespace terion */

#endif /* TERION_COMPONENTS_TAB_PANEL_H */
