/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <memory>
#include <terion/terion.h>

using namespace terion;

int main()
{
    Application app(std::make_unique<AnsiTerminalBackend>());
    State<std::string> choice{"green"};
    app.render([&] {
        return Paper({.children = {
            Typography({.text = "Radio — Tab, Space/Enter or mouse",.variant = TypographyVariant::H2}),
            Radio({.label = "Red",.key = "red",.checked = choice.get() == "red",
                .on_change = [&](bool checked) { if (checked) choice.set("red"); }}),
            Radio({.label = "Green",.key = "green",.checked = choice.get() == "green",
                .on_change = [&](bool checked) { if (checked) choice.set("green"); }}),
            Radio({.label = "Blue",.key = "blue",.checked = choice.get() == "blue",
                .on_change = [&](bool checked) { if (checked) choice.set("blue"); }}),
            Radio({.label = "Disabled",.disabled = true}),
            Typography({.text = "Selected: " + choice.get()})
        },.layout = {.padding = 1,.gap = 1}});
    });
    return app.run();
}
