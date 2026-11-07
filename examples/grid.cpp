/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <memory>
#include <terion/terion.h>

using namespace terion;

int main()
{
    Application app(std::make_unique<AnsiTerminalBackend>());
    app.setTheme(Theme::dark());
    app.render([] {
        return Box({
            .children = {
                Typography({.text = "Terion Grid",.variant = TypographyVariant::H2}),
                Divider({.layout = {.width = 38}}),
                Grid({.children = {
                    Box({.children = {Typography({.text = "CPU"}),Progress({.value = 0.75})},
                        .layout = {.padding = 1},.border = BorderStyle::Rounded}),
                    Box({.children = {Typography({.text = "RAM"}),Progress({.value = 0.45})},
                        .layout = {.padding = 1},.border = BorderStyle::Rounded}),
                    Typography({.text = "Network: OK"}),
                    Typography({.text = "Disk: OK"})
                },.layout = {.width = 44,.gap = 1},.columns = 2})
            },.layout = {.padding = 1,.gap = 1},.border = BorderStyle::Rounded
        });
    });
    return app.run();
}
