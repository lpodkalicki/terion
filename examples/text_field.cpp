/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <memory>
#include <terion/terion.h>

using namespace terion;

int main()
{
    Application app(std::make_unique<AnsiTerminalBackend>());
    State<std::string> name{std::string{}};

    app.render([&] {
        return Column({
            .children = {
                Typography({.text = "Terion TextField",.variant = TypographyVariant::H2}),
                TextField({
                    .value = name.get(),
                    .placeholder = "Enter your name",
                    .key = "name",
                    .layout = {.width = 32},
                    .onChange = [&](std::string value) { name.set(std::move(value)); }
                }),
                Typography({.text = "Hello, " + name.get()}),
                Typography({.text = "Tab, arrows, Home/End, Backspace, Delete, mouse click"})
            },
            .layout = {.padding = 1,.gap = 1}
        });
    });
    return app.run();
}
