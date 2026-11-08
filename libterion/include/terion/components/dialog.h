/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#ifndef TERION_COMPONENTS_DIALOG_H
#define TERION_COMPONENTS_DIALOG_H

#include <terion/component.h>

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
        std::string confirmText = "Confirm";
        std::string cancelText = "Cancel";
        int width = 48;
        bool closeOnBackdrop = true;
        std::function<void()> onConfirm = {};
        std::function<void()> onClose = {};
        std::vector<Component> children = {};
        std::function<void(ComponentEvent&)> onEventCapture = {};
        std::function<void(ComponentEvent&)> onEvent = {};
    }; /* End of struct Props */

    explicit Dialog(Props props) :
        Component({.type = Type::Dialog,.key = std::move(props.key),
            .text = std::move(props.message),.children = std::move(props.children),
            .on_click = std::move(props.onConfirm),
            .on_event_capture = std::move(props.onEventCapture),.on_event = std::move(props.onEvent),
            .dialog_open = props.open,.dialog_title = std::move(props.title),
            .dialog_confirm_text = std::move(props.confirmText),
            .dialog_cancel_text = std::move(props.cancelText),.dialog_width = props.width,
            .dialog_close_on_backdrop = props.closeOnBackdrop,.dialog_on_close = std::move(props.onClose)})
    {}
}; /* End of class Dialog */

} /* End of namespace terion */

#endif /* TERION_COMPONENTS_DIALOG_H */
