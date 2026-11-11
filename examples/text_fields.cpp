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
    State<std::string> name{"Łukasz"};
    State<std::string> search{std::string{}};
    State<std::string> notes{"UTF-8 editing"};
    app.render([&] {
        return Paper({
            .children = {
                Typography({.text = "Terion TextField",.variant = TypographyVariant::H2}),
                Divider({.layout = {.width = 48}}),
                Typography({.text = "Outlined"}),
                TextField({.value = name.get(),.key = "name",.layout = {.width = 38},
                    .on_change = [&](std::string text) { name.set(std::move(text)); }}),
                Typography({.text = "Filled with placeholder"}),
                TextField({.value = search.get(),.placeholder = "Search by name...",.key = "search",
                    .layout = {.width = 38},.on_change = [&](std::string text) { search.set(std::move(text)); },
                    .variant = TextFieldVariant::Filled}),
                Typography({.text = "Compact"}),
                TextField({.value = notes.get(),.key = "notes",.layout = {.width = 38},
                    .on_change = [&](std::string text) { notes.set(std::move(text)); },
                    .size = ButtonSize::Compact}),
                Typography({.text = "Disabled"}),
                TextField({.value = "Read-only value",.disabled = true,.layout = {.width = 38}}),
                Divider({.layout = {.width = 48}}),
                Typography({.text = "Current name: " + name.get()})
            },.layout = {.padding = 1,.gap = 1}
        });
    });
    return app.run();
}
