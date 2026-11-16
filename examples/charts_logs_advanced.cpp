/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <terion/chart_buffer.h>
#include <terion/terion.h>

using namespace terion;

int main()
{
    Application app(std::make_unique<AnsiTerminalBackend>());
    ChartBuffer samples(240,3);
    auto logs = std::make_shared<LogBuffer>(2000);
    auto cache = std::make_shared<LogFilterCache>();
    State<std::vector<Chart::Series>> series({
        {.name="CPU",.values={},.color=ColorVariant::Success},
        {.name="RAM",.values={},.color=ColorVariant::Info},
        {.name="NET",.values={},.color=ColorVariant::Warning}
    });
    State<std::size_t> sample_offset{0};
    State<std::size_t> log_revision{0};
    State<bool> paused_view{false};
    State<std::shared_ptr<const LogBuffer>> frozen_logs{nullptr};
    State<bool> follow{true};
    State<bool> show_grid{true};
    State<LogLevel> minimum_level{LogLevel::Info};
    State<std::string> search{""};
    State<std::optional<std::uint64_t>> selected_log{std::nullopt};
    State<std::optional<std::size_t>> selected_sample{std::nullopt};
    std::size_t tick = 0;

    app.addInterval(std::chrono::milliseconds(150),[&] {
        const double x = static_cast<double>(tick++);
        const double cpu = 50+33*std::sin(x*0.12);
        const double ram = 65+10*std::sin(x*0.027);
        const double net = 35+25*std::abs(std::sin(x*0.21));
        samples.append({cpu,ram,net});
        if (tick % 3 == 0)
        {
            const auto severity = cpu > 75 ? LogLevel::Error : cpu > 65 ?
                LogLevel::Warning : LogLevel::Info;
            logs->append({.timestamp="t="+std::to_string(tick),
                .level=severity,.message="CPU="+std::to_string(static_cast<int>(cpu))+
                    "%  packet="+std::to_string(tick)});
            log_revision.set(static_cast<std::size_t>(logs->revision()));
        }
        series.set({
            {.name="CPU",.values=samples.channel(0),.color=ColorVariant::Success},
            {.name="RAM",.values=samples.channel(1),.color=ColorVariant::Info},
            {.name="NET",.values=samples.channel(2),.color=ColorVariant::Warning}
        });
        sample_offset.set(samples.beginIndex());
    });

    app.render([&] {
        (void)log_revision.get(); // Subscribe to mutations of the shared buffer.
        const auto current = paused_view.get() ? frozen_logs.get() :
            std::shared_ptr<const LogBuffer>(logs);
        const auto filter = LogFilter{minimum_level.get(),search.get(),{}};
        const auto rows = current ? cache->resolve(current,filter)->size() : 0;
        return Paper({.children={
            Typography({.text="Terion | #137b Advanced Chart + LogView",
                .variant=TypographyVariant::H2}),
            Typography({.text="Live monitoring | alarm thresholds | cached log filters | TSV export"}),
            Row({.children={
                Button({.text=paused_view.get()?"Resume view":"Freeze view",
                    .on_click=[paused_view,frozen_logs,logs]() mutable {
                        const bool next=!paused_view.get();
                        frozen_logs.set(next?logs->snapshot():nullptr);
                        paused_view.set(next);
                    }}),
                Button({.text=follow.get()?"Follow: ON":"Follow: OFF",
                    .on_click=[follow]() mutable {follow.set(!follow.get());}}),
                Button({.text=show_grid.get()?"Grid: ON":"Grid: OFF",
                    .on_click=[show_grid]() mutable {show_grid.set(!show_grid.get());}}),
                Button({.text="Min: "+log_detail::levelName(minimum_level.get()),
                    .on_click=[minimum_level]() mutable {
                        const auto level=minimum_level.get();
                        minimum_level.set(level==LogLevel::Info?LogLevel::Warning:
                            level==LogLevel::Warning?LogLevel::Error:LogLevel::Info);
                    }}),
                Button({.text="Copy filtered TSV",.on_click=[paused_view,frozen_logs,logs,minimum_level,search]() mutable {
                    const auto buffer=paused_view.get()?frozen_logs.get():
                        std::shared_ptr<const LogBuffer>(logs);
                    if (buffer) Clipboard::set(LogView::exportTsv(*buffer,
                        {minimum_level.get(),search.get(),{}}));
                }})
            },.layout={.gap=1}}),
            Chart({.series=series.get(),.show_x_axis=true,.show_y_axis=true,.show_legend=true,
                .on_point_click=[selected_sample](std::size_t index) mutable {selected_sample.set(index);},
                .layout={.width=100,.height=15},.variant=Chart::Variant::Line,
                .sample_offset=sample_offset.get(),.cursor=selected_sample.get(),.show_tooltip=true,
                .reference_lines={{.value=75,.label="CPU alarm 75%"}},
                .show_grid=show_grid.get(),.y_unit="%"}),
            Row({.children={
                Typography({.text="Search:"}),
                TextField({.value=search.get(),.placeholder="Filter message, timestamp, level",
                    .key="log-search",.layout={.width=38},
                    .on_change=[search](std::string value) mutable {search.set(std::move(value));}}),
                Typography({.text="Matches: "+std::to_string(rows)+
                    (paused_view.get()?" | FROZEN":" | LIVE")})
            },.layout={.gap=1}}),
            LogView({.buffer=current,.min_level=minimum_level.get(),.search=search.get(),
                .follow_tail=follow.get() && !paused_view.get(),
                .key="log-results",.layout={.width=100,.height=14},
                .filter_cache=cache,.selected_sequence=selected_log.get(),
                .on_select_sequence=[selected_log](std::uint64_t sequence) mutable {
                    selected_log.set(sequence);
                }}),
            Typography({.text="Tab: focus | click: select | wheel: scroll | filter via search | Esc: quit"})
        },.layout={.padding=1,.gap=1}});
    });
    return app.run();
}
