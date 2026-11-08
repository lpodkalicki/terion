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
            Typography({.text = "Checkbox variants — Tab, Space/Enter or mouse",
                .variant = TypographyVariant::H2}),
            Typography({.text = "Default (□ / ■)"}),
            Checkbox({.label = "Default",.key = "default",.checked = standard.get(),
                .onChange = [&](bool checked) { standard.set(checked); }}),
            Checkbox({.label = "Disabled",.key = "default_disabled",.disabled = true}),
            Typography({.text = "Unicode (▢ / ▣)"}),
            Checkbox({.label = "Unicode",.key = "unicode",.checked = unicode.get(),
                .variant = CheckboxVariant::Unicode,
                .onChange = [&](bool checked) { unicode.set(checked); }}),
            Typography({.text = "Emoji (⬜ / ✅)"}),
            Checkbox({.label = "Emoji",.key = "emoji",.checked = emoji.get(),
                .variant = CheckboxVariant::Emoji,
                .onChange = [&](bool checked) { emoji.set(checked); }}),
            Typography({.text = "Toggle each checkbox to compare ON and OFF states."})
        },.layout = {.padding = 1,.gap = 1}});
    });
    return app.run();
}
