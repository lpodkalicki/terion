/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <memory>
#include <terion/terion.h>

using namespace terion;

int main() {
    Application app(std::make_unique<AnsiTerminalBackend>());
    State<int> progress{45};
    app.render([&] {
        return Box({
            .children = {
                Typography({.text = "Terion Dashboard", .variant = TypographyVariant::H2}),
                Divider({.layout = {.width = 36}}),
                Typography({.text = "Downloading..."}),
                Progress({.value = progress.get() / 100.0, .layout = {.width = 30}}),
                Row({
                    .children = {
                        Button({
                            .text = "+10%",
                            .key = "plus",
                            .variant = ButtonVariant::Contained,
                            .onClick = [&] { progress.set(std::min(100, progress.get() + 10)); }
                        }),

                        Button({
                            .text = "Reset",
                            .key = "reset",
                            .variant = ButtonVariant::Outlined,
                            .onClick = [&] { progress.set(0); }
                        })
                    },
                    .layout = {.gap = 2}
                })
            },
            .layout = {.width = 42, .padding = 1, .gap = 1},
            .border = BorderStyle::Rounded,
            .background = Color::rgb(18, 27, 42)
        });
    });
    return app.run();
}
