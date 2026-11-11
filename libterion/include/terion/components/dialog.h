/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#ifndef TERION_COMPONENTS_DIALOG_H
#define TERION_COMPONENTS_DIALOG_H

#include <terion/component.h>
#include <optional>

namespace terion {

class Dialog : public Component
{
public:
    struct Props
    {
        bool open = false;
        std::string title = {};
        std::string message = {};
        std::string key = {};
        std::string confirm_text = "Confirm";
        std::string cancel_text = "Cancel";
        int width = 48;
        bool close_on_backdrop = true;
        std::function<void()> on_confirm = {};
        std::function<void()> on_close = {};
        std::vector<Component> children = {};
        // Defaults to the built-in actions only for dialogs without custom content.
        std::optional<bool> show_actions = std::nullopt;
        std::function<void(ComponentEvent&)> on_event_capture = {};
        std::function<void(ComponentEvent&)> on_event = {};
    }; /* End of struct Props */

    explicit Dialog(Props props) : Component(makeProps(std::move(props))) {}

private:
    static Component::Props makeProps(Props props)
    {
        const bool show_actions = props.show_actions.value_or(props.children.empty());
        return {.type = Type::Dialog,.key = std::move(props.key),
            .text = std::move(props.message),.children = std::move(props.children),
            .on_click = std::move(props.on_confirm),
            .on_event_capture = std::move(props.on_event_capture),.on_event = std::move(props.on_event),
            .dialog_open = props.open,.dialog_title = std::move(props.title),
            .dialog_confirm_text = std::move(props.confirm_text),
            .dialog_cancel_text = std::move(props.cancel_text),.dialog_width = props.width,
            .dialog_close_on_backdrop = props.close_on_backdrop,.dialog_on_close = std::move(props.on_close),
            .dialog_show_actions = show_actions};
    }
}; /* End of class Dialog */

} /* End of namespace terion */

#endif /* TERION_COMPONENTS_DIALOG_H */
