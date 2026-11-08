/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#ifndef TERION_COMPONENTS_TREE_VIEW_H
#define TERION_COMPONENTS_TREE_VIEW_H

#include <terion/component.h>

namespace terion {

class TreeView : public Component
{
public:
    struct Props
    {
        std::vector<TreeItem> items = {};
        std::vector<std::string> expanded = {};
        std::string value = {};
        std::string key = {};
        bool disabled = false;
        LayoutStyle layout = {};
        std::function<void(std::string)> onChange = {};
        std::function<void(std::string,bool)> onToggle = {};
        std::function<void(ComponentEvent&)> onEventCapture = {};
        std::function<void(ComponentEvent&)> onEvent = {};
    }; /* End of struct Props */

    explicit TreeView(Props props) :
        Component({.type = Type::TreeView,.key = std::move(props.key),.text = std::move(props.value),
            .layout = props.layout,.on_event_capture = std::move(props.onEventCapture),
            .on_event = std::move(props.onEvent),.on_change = std::move(props.onChange),
            .disabled = props.disabled,.tree_items = std::move(props.items),
            .tree_expanded = std::move(props.expanded),.tree_on_toggle = std::move(props.onToggle)})
    {}
}; /* End of class TreeView */

} /* End of namespace terion */

#endif /* TERION_COMPONENTS_TREE_VIEW_H */
