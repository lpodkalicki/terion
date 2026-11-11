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
    State<double> volume{45};
    State<double> precision{0.4};

    app.render([&] {
        return Paper({.children = {
            Typography({.text = "Slider — mouse, arrows, PageUp/Down, Home/End",
                .variant = TypographyVariant::H2}),
            Typography({.text = "Volume (0–100, step 5)"}),
            Slider({.value = volume.get(),.min = 0,.max = 100,.step = 5,
                .key = "volume",.on_change = [&](double value) { volume.set(value); },
                .layout = {.width = 38}}),
            Typography({.text = "Gain (0–1, step 0.1)"}),
            Slider({.value = precision.get(),.min = 0,.max = 1,.step = 0.1,
                .key = "gain",.show_value = false,
                .on_change = [&](double value) { precision.set(value); },
                .layout = {.width = 38}}),
            Typography({.text = "Disabled"}),
            Slider({.value = 25,.disabled = true,.layout = {.width = 38}}),
            Typography({.text = "Volume: " + std::to_string(static_cast<int>(volume.get())) +
                "   Gain: " + std::to_string(precision.get())}),
            Typography({.text = "Tab / Shift+Tab changes focus. Drag the handle or click the track."})
        },.layout = {.padding = 1,.gap = 1}});
    });
    return app.run();
}
