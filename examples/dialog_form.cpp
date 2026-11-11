/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <terion/terion.h>

using namespace terion;

int main()
{
    Application app(std::make_unique<AnsiTerminalBackend>());
    State<bool> open(false);
    State<std::string> name("Terion");

    const auto Form = component([&] {
        return Column({.children = {
            Typography({.text = "Application name"}),
            TextField({.value = name.get(),.on_change = [&](std::string value) {
                name.set(std::move(value));
            }}),
            Row({.children = {
                Button({.text = "Cancel",.on_click = [&] { open.set(false); }}),
                Button({.text = "Save",.on_click = [&] { open.set(false); }})
            },.layout = {.gap = 1}})
        },.layout = {.gap = 1}});
    });

    app.render([&] {
        return Column({.children = {
            Typography({.text = "Name: " + name.get()}),
            Button({.text = "Settings",.on_click = [&] { open.set(true); }}),
            Dialog({.open = open.get(),.title = "Settings",
                .on_close = [&] { open.set(false); },.children = {Form()}})
        },.layout = {.padding = 2,.gap = 1}});
    });
    return app.run();
}
