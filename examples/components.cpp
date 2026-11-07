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
    State<int> counter{0};

    app.render([&] {
        return Column({
            .children = {
                Typography({.text = "Terion components",.variant = TypographyVariant::H2}),
                Typography({.text = "Counter: " + std::to_string(counter.get())}),
                Row({
                    .children = {
                        Button({.text = "Increment",.key = "inc",.onClick = [&] {
                            counter.update([](const int value) { return value + 1; });
                        }}),
                        Button({.text = "Reset",.key = "reset",.onClick = [&] { counter.set(0); }}),
                        Button({.text = "Disabled",.disabled = true})
                    },
                    .layout = {.gap = 2}
                }),
                Typography({.text = "Tab, Enter, Space or mouse click; ESC to exit"})
            },
            .layout = {.padding = 1,.gap = 1}
        });
    });

    return app.run();
}
