/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <algorithm>
#include <memory>
#include <terion/terion.h>

using namespace terion;

int main()
{
    Application app(std::make_unique<AnsiTerminalBackend>());
    State<int> sidebar_width{24};
    State<int> editor_height{10};
    app.render([&] {
        const auto screen = app.backend().size();
        return Paper({.children = {
            Typography({.text = "ResizableSplit — drag separator or Tab + arrows / PgUp / PgDn",
                .variant = TypographyVariant::H2}),
            ResizableSplit({.children = {
                Paper({.children = {
                    Typography({.text = "Navigation",.variant = TypographyVariant::H2}),
                    Typography({.text = "Stations"}),
                    Typography({.text = "  Station A"}),
                    Typography({.text = "  Station B"})
                },.layout = {.padding = 1,.gap = 1}}),
                ResizableSplit({.children = {
                    Paper({.children = {
                        Typography({.text = "Editor",.variant = TypographyVariant::H2}),
                        Typography({.text = "Resize both directions using mouse or keyboard."})
                    },.layout = {.padding = 1,.gap = 1}}),
                    Paper({.children = {
                        Typography({.text = "Console",.variant = TypographyVariant::H2}),
                        Typography({.text = "Status: READY"})
                    },.layout = {.padding = 1,.gap = 1}})
                },.direction = SplitDirection::Vertical,.position = editor_height.get(),
                    .minFirst = 4,.minSecond = 3,.key = "vertical-split",
                    .onChange = [&](int value) { editor_height.set(value); }})
            },.direction = SplitDirection::Horizontal,.position = sidebar_width.get(),
                .minFirst = 12,.minSecond = 18,.key = "horizontal-split",
                .layout = {.width = std::max(0,screen.width-4),.height = std::max(0,screen.height-5)},
                .onChange = [&](int value) { sidebar_width.set(value); }}),
            Typography({.text = "Left: " + std::to_string(sidebar_width.get()) +
                " | Top: " + std::to_string(editor_height.get())})
        },.layout = {.padding = 1,.gap = 1}});
    });
    return app.run();
}
