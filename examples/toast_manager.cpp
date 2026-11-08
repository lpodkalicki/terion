/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <memory>
#include <string>
#include <terion/terion.h>

using namespace terion;

int main()
{
    Application app(std::make_unique<AnsiTerminalBackend>());
    State<int> operations{0};

    const auto send = [&](ToastSeverity severity, std::string message, const int duration = 3500) {
        operations.set(operations.get()+1);
        (void)app.toasts().enqueue({.message = std::move(message),
            .severity = severity,.durationMs = duration});
    };

    app.render([&] {
        return Paper({.children = {
            Typography({.text = "ToastManager — notification queue",
                .variant = TypographyVariant::H2}),
            Typography({.text = "Up to 3 notifications appear together. More wait in the queue."}),
            Row({.children = {
                Button({.text = "Import",.key = "import",.variant = ButtonVariant::Contained,
                    .onClick = [&] { send(ToastSeverity::Success,"Files imported"); }}),
                Button({.text = "Save",.key = "save",
                    .onClick = [&] { send(ToastSeverity::Info,"Changes saved"); }}),
                Button({.text = "Warning",.key = "warning",
                    .onClick = [&] { send(ToastSeverity::Warning,"Storage almost full"); }}),
                Button({.text = "Error",.key = "error",
                    .onClick = [&] { send(ToastSeverity::Error,"Operation failed"); }})
            },.layout = {.gap = 1}}),
            Row({.children = {
                Button({.text = "Queue 8",.key = "queue",
                    .onClick = [&] {
                        for (int i = 1; i <= 8; ++i)
                            send(ToastSeverity::Info,"Queued notification " + std::to_string(i),1400);
                    }}),
                Button({.text = "Persistent",.key = "persistent",
                    .onClick = [&] { send(ToastSeverity::Warning,"Click to dismiss",0); }}),
                Button({.text = "Clear all",.key = "clear",
                    .onClick = [&] { app.toasts().clear(); }})
            },.layout = {.gap = 1}}),
            Typography({.text = "Operations: " + std::to_string(operations.get())}),
            Typography({.text = "Click a toast to dismiss it and show the next one."})
        },.layout = {.padding = 1,.gap = 1}});
    });
    return app.run();
}
