/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <terion/terion.h>

using namespace terion;

namespace {

Component PalettePreview()
{
    return Column({.children = {
        Typography({.text = "Semantic colors",.variant = TypographyVariant::H2}),
        Typography({.text = "Success: operation complete",.color = ColorVariant::Success}),
        Typography({.text = "Warning: check configuration",.color = ColorVariant::Warning}),
        Typography({.text = "Error: connection failed",.color = ColorVariant::Error}),
        Typography({.text = "Info: new version available",.color = ColorVariant::Info}),
        Row({.children = {
            Button({.text = "Primary",.variant = ButtonVariant::Contained}),
            Button({.text = "Success",.variant = ButtonVariant::Contained,.color = ColorVariant::Success}),
            Button({.text = "Warning",.variant = ButtonVariant::Contained,.color = ColorVariant::Warning})
        },.layout = {.gap = 1}}),
        Row({.children = {
            Button({.text = "Error",.variant = ButtonVariant::Outlined,.color = ColorVariant::Error}),
            Button({.text = "Info",.variant = ButtonVariant::Text,.color = ColorVariant::Info})
        },.layout = {.gap = 1}}),
        Progress({.value = 0.7,.color = ColorVariant::Success}),
        Progress({.value = 0.4,.color = ColorVariant::Warning}),
        Checkbox({.label = "Accept changes",.checked = true,.color = ColorVariant::Success}),
        Switch({.label = "Monitoring enabled",.checked = true,.color = ColorVariant::Info}),
        Radio({.label = "Selected",.checked = true,.color = ColorVariant::Warning}),
        RadioGroup({.options = {{.label = "First",.value = "first"},
            {.label = "Second",.value = "second"}},.value = "second",
            .color = ColorVariant::Error}),
        Slider({.value = 65,.color = ColorVariant::Success})
    },.layout = {.padding = 2,.gap = 1}});
}

} /* End of anonymous namespace */

int main()
{
    Application app(std::make_unique<AnsiTerminalBackend>());
    State<bool> light(false);
    app.render([&] {
        return ThemeProvider(light.get() ? Theme::light() : Theme::dark(),
            Column({.children = {
                Button({.text = "Toggle light/dark",.on_click = [&] { light.set(!light.get()); }}),
                PalettePreview()
            }}));
    });
    return app.run();
}
