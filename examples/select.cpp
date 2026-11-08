/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <terion/terion.h>

using namespace terion;

int main()
{
    Application app(std::make_unique<AnsiTerminalBackend>());
    State<std::string> selected("pl");
    app.render([&] {
        return Column({.children = {
            Typography({.text = "Terion Select — Enter/Space, arrows, mouse, wheel, Esc"}),
            Select({.options = {
                {.label = "Polski",.value = "pl"},
                {.label = "English",.value = "en"},
                {.label = "Deutsch",.value = "de"},
                {.label = "Français",.value = "fr"},
                {.label = "Español",.value = "es"},
                {.label = "Italiano",.value = "it"},
                {.label = "Português",.value = "pt"},
                {.label = "Unavailable",.value = "disabled",.disabled = true}
            },.value = selected.get(),.placeholder = "Select language",.key = "language",
                .layout = {.width = 28},.onChange = [&](std::string value) { selected.set(std::move(value)); }}),
            Typography({.text = "Selected value: " + selected.get()})
        },.layout = {.padding = 2,.gap = 1}});
    });
    return app.run();
}
