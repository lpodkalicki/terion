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
            TextField({.value = name.get(),.onChange = [&](std::string value) {
                name.set(std::move(value));
            }}),
            Row({.children = {
                Button({.text = "Cancel",.onClick = [&] { open.set(false); }}),
                Button({.text = "Save",.onClick = [&] { open.set(false); }})
            },.layout = {.gap = 1}})
        },.layout = {.gap = 1}});
    });

    app.render([&] {
        return Column({.children = {
            Typography({.text = "Name: " + name.get()}),
            Button({.text = "Settings",.onClick = [&] { open.set(true); }}),
            Dialog({.open = open.get(),.title = "Settings",
                .onClose = [&] { open.set(false); },.children = {Form()}})
        },.layout = {.padding = 2,.gap = 1}});
    });
    return app.run();
}
