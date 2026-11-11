/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#ifndef TERION_COMPONENTS_SCROLL_VIEW_H
#define TERION_COMPONENTS_SCROLL_VIEW_H

#include <terion/component.h>

namespace terion {

class ScrollView : public Component
{
public:
    struct Props
    {
        std::vector<Component> children = {};
        std::string key = {};
        LayoutStyle layout = {};
        int scroll_offset = 0;
        std::function<void(int)> on_scroll = {};
        bool show_scrollbar = true;
        std::function<void(ComponentEvent&)> on_event_capture = {};
        std::function<void(ComponentEvent&)> on_event = {};
    }; /* End of struct Props */

    explicit ScrollView(Props props) :
        Component({.type = Type::ScrollView,.key = std::move(props.key),
            .children = std::move(props.children),.layout = props.layout,
            .on_event_capture = std::move(props.on_event_capture),.on_event = std::move(props.on_event),
            .scroll_offset = props.scroll_offset,.on_scroll = std::move(props.on_scroll),
            .scrollbar_visible = props.show_scrollbar})
    {}
}; /* End of class ScrollView */

} /* End of namespace terion */

#endif /* TERION_COMPONENTS_SCROLL_VIEW_H */
