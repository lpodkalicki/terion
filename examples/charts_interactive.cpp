/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <cmath>
#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>
#include <terion/terion.h>

using namespace terion;

int main()
{
    Application app(std::make_unique<AnsiTerminalBackend>());
    State<std::vector<Chart::Series>> metrics({
        {.name = "CPU",.values = {},.color = ColorVariant::Success},
        {.name = "RAM",.values = {},.color = ColorVariant::Info},
        {.name = "NET",.values = {},.color = ColorVariant::Warning}
    });
    State<std::pair<std::size_t,std::size_t>> viewport({0,0});
    State<int> selected{-1};
    State<int> mode{0};
    State<bool> follow{true};
    for (std::size_t i = 0; i < 120; ++i)
    {
        metrics.update([i](std::vector<Chart::Series> series) {
            const double t = static_cast<double>(i);
            series[0].values.push_back(45+24*std::sin(t*0.12));
            series[1].values.push_back(58+8*std::sin(t*0.06));
            series[2].values.push_back(25+36*std::abs(std::sin(t*0.2)));
            return series;
        });
    }
    app.render([&] {
        const auto series = metrics.get();
        const auto total = Chart::sampleCount(series);
        auto view = viewport.get();
        if (follow.get() && total)
        {
            const auto window = view.second == 0 ? std::size_t{60} : view.second;
            view = Chart::clampView(total,total > window ? total-window : 0,window);
        }
        auto change_view = [follow,viewport](std::size_t begin,std::size_t count) mutable {
            follow.set(false);
            viewport.set({begin,count});
        };
        auto zoom_by = [total,view,change_view](int direction) mutable {
            const auto next = Chart::zoom(total,view.first,view.second,direction);
            change_view(next.first,next.second);
        };
        auto pan_by = [total,view,change_view](int direction) mutable {
            const auto next = Chart::pan(total,view.first,view.second,direction);
            change_view(next.first,next.second);
        };
        return Paper({.children = {
            Typography({.text = "Terion | #133b Interactive Chart",.variant = TypographyVariant::H2}),
            Typography({.text = "Click: inspect | Wheel: zoom | Shift+wheel: pan"}),
            Row({.children = {
                Button({.text = "Zoom +",.on_click = [zoom_by]() mutable { zoom_by(1); }}),
                Button({.text = "Zoom -",.on_click = [zoom_by]() mutable { zoom_by(-1); }}),
                Button({.text = "< Pan",.on_click = [pan_by]() mutable { pan_by(-1); }}),
                Button({.text = "Pan >",.on_click = [pan_by]() mutable { pan_by(1); }}),
                Button({.text = "Follow tail",.on_click = [follow]() mutable { follow.set(true); }}),
                Button({.text = "Add sample",.on_click = [metrics]() mutable {
                    metrics.update([](std::vector<Chart::Series> next) {
                        const double t = static_cast<double>(next[0].values.size());
                        next[0].values.push_back(45+24*std::sin(t*0.12));
                        next[1].values.push_back(58+8*std::sin(t*0.06));
                        next[2].values.push_back(25+36*std::abs(std::sin(t*0.2)));
                        return next;
                    });
                }})
            },.layout = {.gap = 1}}),
            Row({.children = {
                Button({.text = "Line",.on_click = [mode]() mutable { mode.set(0); }}),
                Button({.text = "Scatter",.on_click = [mode]() mutable { mode.set(1); }}),
                Button({.text = "Bars",.on_click = [mode]() mutable { mode.set(2); }})
            },.layout = {.gap = 1}}),
            Chart({.series = series,.show_x_axis = true,.show_y_axis = true,.show_legend = true,
                .on_point_click = [selected](std::size_t index) mutable { selected.set(static_cast<int>(index)); },
                .layout = {.width = 88,.height = 19},
                .variant = static_cast<Chart::Variant>(mode.get()),
                .view_begin = view.first,.view_count = view.second,
                .cursor = selected.get() < 0 ? std::nullopt :
                    std::optional<std::size_t>{static_cast<std::size_t>(selected.get())},
                .show_tooltip = true,
                .on_view_change = change_view}),
            Typography({.text = "Samples: " + std::to_string(total) +
                " | Window: " + std::to_string(view.first) +
                " .. " + std::to_string(view.first+view.second) +
                (follow.get() ? " | FOLLOW" : " | PAUSED")})
        },.layout = {.padding = 1,.gap = 1}});
    });
    return app.run();
}
