/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#ifndef TERION_COMPONENTS_VIRTUAL_LIST_H
#define TERION_COMPONENTS_VIRTUAL_LIST_H

#include <algorithm>
#include <functional>
#include <string>
#include <terion/component.h>

namespace terion {

// Fixed-height, text-row virtual list. The callback is evaluated only for rows
// currently visible in the viewport, not for the entire dataset.
class VirtualList : public Component
{
public:
    struct Props
    {
        int item_count = 0;
        std::function<std::string(int)> item_text = {};
        std::function<StyledText(int)> item_styled = {};
        int selected_index = -1;
        int row_height = 1;
        bool follow_tail = false;
        std::function<void(int)> on_select = {};
        std::function<void(int)> on_activate = {};
        std::string key = {};
        LayoutStyle layout = {};
        bool disabled = false;
        bool show_scrollbar = true;
        std::function<void(ComponentEvent&)> on_event_capture = {};
        std::function<void(ComponentEvent&)> on_event = {};
    }; /* End of struct Props */

    explicit VirtualList(Props props) :
        Component({.type = Type::VirtualList,.key = std::move(props.key),
            .layout = props.layout,.on_event_capture = std::move(props.on_event_capture),
            .on_event = std::move(props.on_event),.disabled = props.disabled,
            .scrollbar_visible = props.show_scrollbar,
            .virtual_item_count = std::max(0,props.item_count),
            .virtual_item_text = std::move(props.item_text),
            .virtual_item_styled = std::move(props.item_styled),
            .virtual_selected_index = props.selected_index,
            .virtual_row_height = std::max(1,props.row_height),
            .virtual_follow_tail = props.follow_tail,
            .virtual_on_select = std::move(props.on_select),
            .virtual_on_activate = std::move(props.on_activate)})
    {}
}; /* End of class VirtualList */

} /* End of namespace terion */

#endif /* TERION_COMPONENTS_VIRTUAL_LIST_H */
