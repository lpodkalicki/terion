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
        int scrollOffset = 0;
        std::function<void(int)> onScroll = {};
        bool showScrollbar = true;
    }; /* End of struct Props */

    explicit ScrollView(Props props) :
        Component({.type = Type::ScrollView,.key = std::move(props.key),
            .children = std::move(props.children),.layout = props.layout,
            .scroll_offset = props.scrollOffset,.on_scroll = std::move(props.onScroll),
            .scrollbar_visible = props.showScrollbar})
    {}
}; /* End of class ScrollView */

} /* End of namespace terion */

#endif /* TERION_COMPONENTS_SCROLL_VIEW_H */
