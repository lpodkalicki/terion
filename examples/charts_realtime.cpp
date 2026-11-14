/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>
#include <terion/terion.h>
#include <terion/chart_buffer.h>

using namespace terion;

int main()
{
    Application app(std::make_unique<AnsiTerminalBackend>());
    constexpr std::size_t capacity=300;
    ChartBuffer buffer(capacity,3);
    State<std::vector<Chart::Series>> samples({
        {.name="CPU",.values={},.color=ColorVariant::Success},
        {.name="RAM",.values={},.color=ColorVariant::Info},
        {.name="NET",.values={},.color=ColorVariant::Warning}
    });
    State<std::size_t> first_index{0};
    State<std::size_t> received{0};
    State<std::pair<std::size_t,std::size_t>> viewport{{0,80}};
    State<bool> paused{false};
    State<bool> follow{true};
    State<bool> independent{true};
    State<int> fps{10};
    State<std::optional<std::size_t>> selected{std::nullopt};
    std::size_t tick=0;
    bool dirty=false;
    auto latest=std::chrono::steady_clock::now();

    // 50 samples/s: constant-memory ingestion; never touches UI from another thread.
    app.addInterval(std::chrono::milliseconds(20),[&] {
        if (paused.get()) return;
        const double t=static_cast<double>(tick++);
        buffer.append({52.0+28.0*std::sin(t*0.045),
            60.0+12.0*std::sin(t*0.016),
            180.0+90.0*std::abs(std::sin(t*0.11))});
        dirty=true;
    });
    // Throttle state changes to ~10 FPS by default (configurable in toolbar).
    app.addInterval(std::chrono::milliseconds(10),[&] {
        if (!dirty) return;
        const auto now=std::chrono::steady_clock::now();
        if (now-latest<std::chrono::milliseconds(1000/std::max(1,fps.get()))) return;
        latest=now;
        dirty=false;
        samples.set({
            {.name="CPU",.values=buffer.channel(0),.color=ColorVariant::Success},
            {.name="RAM",.values=buffer.channel(1),.color=ColorVariant::Info},
            {.name="NET",.values=buffer.channel(2),.color=ColorVariant::Warning}
        });
        first_index.set(buffer.beginIndex());
        received.set(buffer.endIndex());
    });

    app.render([&] {
        const auto series=samples.get();
        const auto count=Chart::sampleCount(series);
        const auto base=first_index.get();
        auto view=viewport.get();
        if (follow.get())
        {
            const auto width=std::max(std::size_t{2},view.second);
            view=Chart::clampView(count,count>width ? count-width : 0,width);
        }
        else
        {
            // Preserve global position when old samples are evicted.
            const auto local_begin=view.first>base ? view.first-base : 0;
            view=Chart::clampView(count,local_begin,view.second);
        }
        const auto selected_global=selected.get();
        const auto cursor=selected_global && *selected_global>=base ?
            std::optional<std::size_t>{*selected_global-base} : std::nullopt;
        const auto local_change=[viewport,follow,base](std::size_t begin,std::size_t size) mutable {
            follow.set(false);
            viewport.set({base+begin,size});
        };
        auto change_zoom=[viewport,follow,count,view,base](int delta) mutable {
            const auto next=Chart::zoom(count,view.first,view.second,delta);
            follow.set(false);
            viewport.set({base+next.first,next.second});
        };
        auto change_pan=[viewport,follow,count,view,base](int delta) mutable {
            const auto next=Chart::pan(count,view.first,view.second,delta);
            follow.set(false);
            viewport.set({base+next.first,next.second});
        };
        return Paper({.children={
            Typography({.text="Terion | #133c Real-Time Charts",.variant=TypographyVariant::H2}),
            Typography({.text="50 samples/s | ring buffer 300 | renderer throttled | click and wheel to inspect"}),
            Row({.children={
                Button({.text=paused.get()?"Resume":"Pause",.on_click=[paused]() mutable {paused.set(!paused.get());}}),
                Button({.text="Follow",.on_click=[follow]() mutable {follow.set(true);}}),
                Button({.text="Zoom +",.on_click=[change_zoom]() mutable {change_zoom(1);}}),
                Button({.text="Zoom -",.on_click=[change_zoom]() mutable {change_zoom(-1);}}),
                Button({.text="< Pan",.on_click=[change_pan]() mutable {change_pan(-1);}}),
                Button({.text="Pan >",.on_click=[change_pan]() mutable {change_pan(1);}}),
                Button({.text="Independent Y",.on_click=[independent]() mutable {independent.set(!independent.get());}}),
                Button({.text="FPS 5/10/20",.on_click=[fps]() mutable {fps.set(fps.get()==5?10:fps.get()==10?20:5);}})
            },.layout={.gap=1}}),
            Chart({.series=series,.show_x_axis=true,.show_y_axis=true,.show_legend=true,
                .on_point_click=[selected,base](std::size_t index) mutable {selected.set(base+index);},
                .layout={.width=96,.height=20},.variant=Chart::Variant::Line,
                .independent_y_axes=independent.get(),.sample_offset=base,
                .view_begin=view.first,.view_count=view.second,.cursor=cursor,
                .show_tooltip=true,.on_view_change=local_change}),
            Typography({.text="Received: "+std::to_string(received.get())+
                " | Retained: "+std::to_string(count)+"/"+std::to_string(capacity)+
                " | FPS: "+std::to_string(fps.get())+
                (follow.get()?" | FOLLOW":" | MANUAL")+
                (independent.get()?" | PER-SERIES Y":" | SHARED Y")})
        },.layout={.padding=1,.gap=1}});
    });
    return app.run();
}
