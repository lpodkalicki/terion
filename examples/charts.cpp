/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <cmath>
#include <memory>
#include <vector>
#include <terion/terion.h>

using namespace terion;

int main()
{
    Application app(std::make_unique<AnsiTerminalBackend>());
    State<int> mode{0};
    std::vector<double> wave;
    std::vector<double> trend;
    for (int i = 0; i < 100; ++i)
    {
        wave.push_back(std::sin(i*0.18)+0.3*std::cos(i*0.47));
        trend.push_back(static_cast<double>(i)/15.0+std::sin(i*0.27));
    }
    app.render([&] {
        const auto variant = static_cast<Chart::Variant>(mode.get());
        return Paper({.children = {
            Typography({.text = "Terion | #133 Terminal Charts",.variant = TypographyVariant::H2}),
            Typography({.text = "Compact trend"}),
            Sparkline({.values = wave,.layout = {.width = 60,.height = 1}}),
            Typography({.text = "ASCII fallback"}),
            Sparkline({.values = trend,.layout = {.width = 60,.height = 1},
                .variant = Sparkline::Variant::Ascii}),
            Row({.children = {
                Button({.text = "Line",.on_click = [&] { mode.set(0); }}),
                Button({.text = "Scatter",.on_click = [&] { mode.set(1); }}),
                Button({.text = "Bars",.on_click = [&] { mode.set(2); }})
            },.layout = {.gap = 1}}),
            Chart({.values = wave,.layout = {.width = 60,.height = 12},.variant = variant}),
            Typography({.text = "Braille chart (2x4 pixel resolution per terminal cell)"}),
            Typography({.text = "Tab: focus | Enter: select mode | Esc: quit"})
        },.layout = {.padding = 1,.gap = 1}});
    });
    return app.run();
}
