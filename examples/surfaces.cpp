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
        return Stack({
            .children = {
                Typography({.text = "Terion Surfaces",.variant = TypographyVariant::H2}),
                Grid({.children = {
                    Card({.title = "CPU",.children = {
                        Typography({.text = "75% utilization"}),
                        Progress({.value = 0.75,.layout = {.width = 14}})
                    },.layout = {.width = 22,.padding = 1,.gap = 1}}),
                    Card({.title = "Memory",.children = {
                        Typography({.text = "45% allocated"}),
                        Progress({.value = 0.45,.layout = {.width = 14}})
                    },.layout = {.width = 22,.padding = 1,.gap = 1}})
                },.layout = {.width = 46,.gap = 2},.columns = 2}),
                Paper({.children = {Typography({.text = "All systems operational"})},
                    .layout = {.width = 46,.padding = 1}})
            },.layout = {.gap = 1}
        });
    });
    return app.run();
}
