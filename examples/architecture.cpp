/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <terion/terion.h>

using namespace terion;

int main()
{
    Application app(std::make_unique<AnsiTerminalBackend>());
    State<bool> dialog_open(false);
    State<std::string> name("Terion");
    State<int> saves(0);
    State<std::string> last_event("None");

    app.render([&] {
        return Column({.children = {
            Typography({.text = "Terion — event propagation, focus scopes, composition"}),
            Box({.children = {
                Button({.text = "Edit profile",.key = "open",.variant = ButtonVariant::Contained,
                    .onClick = [&] { dialog_open.set(true); }})
            },.layout = {.padding = 1},.onEventCapture = [&](ComponentEvent& event) {
                if (event.event.key == Key::Enter) last_event.set("Capture: Enter");
            },.onEvent = [&](ComponentEvent& event) {
                if (event.event.key == Key::Enter) last_event.set("Bubble: Enter");
            }}),
            Typography({.text = "Name: " + name.get()}),
            Typography({.text = "Saved: " + std::to_string(saves.get())}),
            Typography({.text = "Last event: " + last_event.get()}),
            Dialog({.open = dialog_open.get(),.title = "Profile",.message = "Edit and save your name",
                .key = "editor",.onClose = [&] { dialog_open.set(false); },
                .children = {
                    TextField({.value = name.get(),.placeholder = "Name",.key = "name",
                        .onChange = [&](std::string value) { name.set(std::move(value)); },
                        .size = ButtonSize::Compact}),
                    Button({.text = "Save",.key = "save",.variant = ButtonVariant::Contained,
                        .onClick = [&] { saves.set(saves.get()+1); dialog_open.set(false); },
                        .size = ButtonSize::Compact})
                }})
        },.layout = {.padding = 2,.gap = 1}});
    });
    return app.run();
}
