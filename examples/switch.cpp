/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <memory>
#include <terion/terion.h>

using namespace terion;

int main()
{
    Application app(std::make_unique<AnsiTerminalBackend>());
    State<bool> standard{true};
    State<bool> unicode{false};
    State<bool> emoji{true};
    app.render([&] {
        return Paper({.children = {
            Typography({.text = "Switch variants — Tab, Space/Enter or mouse",
                .variant = TypographyVariant::H2}),
            Typography({.text = "Default"}),
            Switch({.label = "Default",.key = "default",.checked = standard.get(),
                .onChange = [&](bool checked) { standard.set(checked); }}),
            Switch({.label = "Disabled",.key = "disabled",.checked = true,.disabled = true}),
            Typography({.text = "Unicode"}),
            Switch({.label = "Unicode",.key = "unicode",.checked = unicode.get(),
                .variant = CheckboxVariant::Unicode,
                .onChange = [&](bool checked) { unicode.set(checked); }}),
            Typography({.text = "Emoji"}),
            Switch({.label = "Emoji",.key = "emoji",.checked = emoji.get(),
                .variant = CheckboxVariant::Emoji,
                .onChange = [&](bool checked) { emoji.set(checked); }}),
            Typography({.text = "Toggle each switch to compare ON and OFF states."})
        },.layout = {.padding = 1,.gap = 1}});
    });
    return app.run();
}
