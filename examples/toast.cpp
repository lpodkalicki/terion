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
    State<bool> open{false};
    State<int> sequence{0};
    State<std::string> message{""};
    State<ToastSeverity> severity{ToastSeverity::Info};

    const auto notify = [&](ToastSeverity kind, std::string text) {
        severity.set(kind);
        message.set(std::move(text));
        sequence.set(sequence.get()+1);
        open.set(true);
    };

    app.render([&] {
        return Paper({.children = {
            Typography({.text = "Toast — notifications after file operations",
                .variant = TypographyVariant::H2}),
            Typography({.text = "Select an operation to display a notification."}),
            Row({.children = {
                Button({.text = "Import",.key = "import",.variant = ButtonVariant::Contained,
                    .on_click = [&] { notify(ToastSeverity::Success,"3 files imported successfully"); }}),
                Button({.text = "Save",.key = "save",
                    .on_click = [&] { notify(ToastSeverity::Info,"Changes saved"); }}),
                Button({.text = "Low space",.key = "warning",
                    .on_click = [&] { notify(ToastSeverity::Warning,"Storage almost full"); }}),
                Button({.text = "Failed",.key = "error",
                    .on_click = [&] { notify(ToastSeverity::Error,"Could not delete file"); }})
            },.layout = {.gap = 1}}),
            Typography({.text = "Toast appears in the bottom-right corner."}),
            Typography({.text = "It closes automatically or when clicked."}),
            Typography({.text = "Tab / Shift+Tab to navigate; Enter to activate."}),
            Toast({.open = open.get(),.message = message.get(),.severity = severity.get(),
                .duration_ms = 3500,.key = "toast-" + std::to_string(sequence.get()),
                .on_close = [&] { open.set(false); }})
        },.layout = {.padding = 1,.gap = 1}});
    });
    return app.run();
}
