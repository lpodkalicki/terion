/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <memory>
#include <terion/terion.h>

using namespace terion;

int main()
{
    Application app(std::make_unique<AnsiTerminalBackend>());
    State<std::string> text{std::string("Hello, Terion!\nEdit this document.\nUTF-8: Łódź 日本語")};
    app.render([&] {
        return Column({
            .children = {
                Typography({.text = "TextArea — patch #123",.variant = TypographyVariant::H2}),
                TextArea({.value = text.get(),.placeholder = "Write something...",
                    .key = "editor",
                    .layout = {.width = 60,.height = 12},
                    .on_change = [&](std::string value) { text.set(std::move(value)); }}),
                Typography({.text = "Arrows / Shift+arrows: move/select | Enter: newline | Ctrl+A: select all"}),
                Typography({.text = "Mouse: place cursor | Paste: multiline | Tab: focus next"})
            },
            .layout = {.padding = 1,.gap = 1}
        });
    });
    return app.run();
}
