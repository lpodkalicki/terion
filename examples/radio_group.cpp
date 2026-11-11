/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <memory>
#include <terion/terion.h>

using namespace terion;

int main()
{
    Application app(std::make_unique<AnsiTerminalBackend>());
    State<std::string> mode{"auto"};
    app.render([&] {
        return Paper({.children = {
            Typography({.text = "RadioGroup — Tab, arrows, Space/Enter or mouse",
                .variant = TypographyVariant::H2}),
            RadioGroup({.options = {{"Automatic","auto"},{"Manual","manual"},
                {"Unavailable","locked",true},{"Off","off"}},
                .value = mode.get(),.key = "mode",.on_change = [&](std::string value) { mode.set(std::move(value)); }}),
            Typography({.text = "Selected mode: " + mode.get()})
        },.layout = {.padding = 1,.gap = 1}});
    });
    return app.run();
}
