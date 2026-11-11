/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <algorithm>
#include <memory>
#include <string>
#include <vector>
#include <terion/terion.h>

using namespace terion;

int main()
{
    Application app(std::make_unique<AnsiTerminalBackend>());
    State<int> offset{0};
    app.render([&] {
        const Size screen = app.backend().size();
        std::vector<Component> entries;
        entries.reserve(60);
        for (int i = 0; i < 60; ++i)
        {
            entries.emplace_back(Paper({
                .children = {
                    Typography({.text = "Entry " + std::to_string(i + 1),.variant = TypographyVariant::Subtitle}),
                    Typography({.text = "This item remains inside the clipped ScrollView viewport."})
                },
                .key = "entry-" + std::to_string(i),
                .layout = {.width = std::max(0,screen.width - 4),.padding = 1,.gap = 0},
                .variant = PaperVariant::Outlined
            }));
        }
        return Column({
            .children = {
                Typography({.text = "TERION / ScrollView",.variant = TypographyVariant::H2}),
                Typography({.text = "Drag scrollbar / Wheel / Up / Down / PgUp / PgDn / Home / End  |  Offset: " +
                    std::to_string(offset.get())}),
                Divider({.layout = {.width = screen.width}}),
                ScrollView({
                    .children = std::move(entries),
                    .key = "list",
                    .layout = {.width = screen.width,.gap = 1,.flex = 1},
                    .scroll_offset = offset.get(),
                    .on_scroll = [&](int next) { offset.set(next); }
                }),
                Row({.children = {
                    Button({.text = "Top",.key = "top",.variant = ButtonVariant::Contained,
                        .on_click = [&] { offset.set(0); },.size = ButtonSize::Compact}),
                    Typography({.text = "60 items | ScrollView clips content to its viewport"})
                },.layout = {.gap = 2}})
            },
            .layout = {.width = screen.width,.height = screen.height,.gap = 1,
                .align_items = AlignItems::Stretch}
        });
    });
    return app.run();
}
