/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#ifndef TERION_COMPONENTS_TOAST_H
#define TERION_COMPONENTS_TOAST_H

#include <terion/component.h>

namespace terion {

enum class ToastSeverity
{
    Info,
    Success,
    Warning,
    Error
}; /* End of enum ToastSeverity */

// A non-modal overlay. durationMs == 0 means the toast remains until closed.
class Toast : public Component
{
public:
    struct Props
    {
        bool open = false;
        std::string message = {};
        ToastSeverity severity = ToastSeverity::Info;
        int duration_ms = 3500;
        std::string key = {};
        std::function<void()> on_close = {};
    }; /* End of struct Props */

    explicit Toast(Props props) :
        Component({.type = Type::Toast,.key = std::move(props.key),
            .text = std::move(props.message),.toast_open = props.open,
            .toast_duration_ms = props.duration_ms,
            .toast_severity = static_cast<int>(props.severity),
            .toast_on_close = std::move(props.on_close)})
    {}
}; /* End of class Toast */

} /* End of namespace terion */

#endif /* TERION_COMPONENTS_TOAST_H */
