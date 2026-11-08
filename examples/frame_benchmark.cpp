/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <chrono>
#include <iomanip>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <terion/terion.h>

using namespace terion;

namespace {

double milliseconds(const std::chrono::nanoseconds duration)
{
    return std::chrono::duration<double,std::milli>(duration).count();
}

void benchmark(const int count)
{
    auto backend = std::make_unique<HeadlessTerminalBackend>(Size{.width = 120,.height = 40});
    auto* terminal = backend.get();
    Application app(std::move(backend));
    int changed = 0;
    app.render([&] {
        std::vector<Component> children;
        children.reserve(static_cast<std::size_t>(count));
        for (int i = 0; i < count; ++i)
            children.emplace_back(Component::text("Row " + std::to_string(i) +
                (i == 0 ? ": " + std::to_string(changed) : ""),"row-" + std::to_string(i)));
        return Component::column(std::move(children));
    });

    constexpr int iterations = 20;
    Application::FrameTimings total{};
    app.renderOnce(); // Warm-up includes allocation of the retained tree.
    for (int i = 0; i < iterations; ++i)
    {
        ++changed;
        app.renderOnce();
        const auto timings = app.lastFrameTimings();
        total.build += timings.build;
        total.reconcile += timings.reconcile;
        total.layout += timings.layout;
        total.paint += timings.paint;
        total.terminal += timings.terminal;
        total.total += timings.total;
        terminal->clearOutput();
    }

    const auto avg = [](const std::chrono::nanoseconds value) {
        return milliseconds(value)/iterations;
    };
    std::cout << std::setw(6) << count << "  " << std::fixed << std::setprecision(3)
        << std::setw(8) << avg(total.build) << "  "
        << std::setw(8) << avg(total.reconcile) << "  "
        << std::setw(8) << avg(total.layout) << "  "
        << std::setw(8) << avg(total.paint) << "  "
        << std::setw(8) << avg(total.terminal) << "  "
        << std::setw(8) << avg(total.total) << '\n';
}

} /* End of anonymous namespace */

int main()
{
    std::cout << "Full frame benchmark (headless terminal, average milliseconds)\n"
        << " Nodes     Build   Reconcile    Layout     Paint  Terminal     Total\n";
    for (int count : {100,1000,10000}) benchmark(count);
}
