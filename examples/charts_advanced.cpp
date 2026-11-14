/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <cmath>
#include <memory>
#include <string>
#include <vector>
#include <terion/terion.h>

using namespace terion;

int main()
{
    Application app(std::make_unique<AnsiTerminalBackend>());
    State<int> variant{0};
    State<int> selected{-1};
    std::vector<double> cpu;
    std::vector<double> ram;
    std::vector<double> network;
    for (int i = 0; i < 120; ++i)
    {
        cpu.push_back(40.0 + 27.0*std::sin(i*0.12) + 6.0*std::cos(i*0.39));
        ram.push_back(30.0 + i*0.35 + 4.0*std::sin(i*0.08));
        network.push_back(20.0 + 38.0*std::abs(std::sin(i*0.23)));
    }

    app.render([&] {
        const auto mode = static_cast<Chart::Variant>(variant.get());
        return Paper({.children = {
            Typography({.text = "Terion | #133a Advanced Charts",.variant = TypographyVariant::H2}),
            Typography({.text = "Multi-series metrics dashboard - click chart to select sample"}),
            Row({.children = {
                Button({.text = "Line",.on_click = [&] { variant.set(0); }}),
                Button({.text = "Scatter",.on_click = [&] { variant.set(1); }}),
                Button({.text = "Bars",.on_click = [&] { variant.set(2); }})
            },.layout = {.gap = 1}}),
            Chart({.series = {
                {.name = "CPU",.values = cpu,.color = ColorVariant::Success},
                {.name = "RAM",.values = ram,.color = ColorVariant::Info},
                {.name = "NET",.values = network,.color = ColorVariant::Warning}},
                .show_x_axis = true,.show_y_axis = true,.show_legend = true,
                .on_point_click = [&](std::size_t index) { selected.set(static_cast<int>(index)); },
                .layout = {.width = 80,.height = 18},.variant = mode}),
            Typography({.text = selected.get() < 0 ? "Select a sample by clicking the plot" :
                "Selected sample: " + std::to_string(selected.get())}),
            Typography({.text = "Tab: focus | Enter: change chart mode | Esc: quit"})
        },.layout = {.padding = 1,.gap = 1}});
    });
    return app.run();
}
