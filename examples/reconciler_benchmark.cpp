/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>
#include <terion/terion.h>

using namespace terion;

namespace {

Component makeTree(const std::vector<int>& order)
{
    std::vector<Component> children;
    children.reserve(order.size());
    for (int index : order)
        children.emplace_back(Component::text("Item " + std::to_string(index),
            "row-" + std::to_string(index)));
    return Component::column(std::move(children));
}

void benchmark(const int count)
{
    std::vector<int> order(static_cast<std::size_t>(count));
    std::iota(order.begin(),order.end(),0);
    Reconciler reconciler;
    constexpr int iterations = 30;

    auto measure = [&](const char* label, const bool reverse) {
        std::chrono::nanoseconds total{};
        Reconciler::Stats stats;
        for (int i = 0; i < iterations; ++i)
        {
            if (reverse) std::reverse(order.begin(),order.end());
            const auto component = makeTree(order);
            const auto start = std::chrono::steady_clock::now();
            reconciler.update(component);
            total += std::chrono::steady_clock::now()-start;
            stats = reconciler.stats();
        }
        const double milliseconds = std::chrono::duration<double,std::milli>(total).count()/iterations;
        std::cout << std::setw(6) << count << "  " << std::setw(9) << label << "  "
            << std::fixed << std::setprecision(3) << std::setw(9) << milliseconds
            << " ms  created=" << stats.created << " reused=" << stats.reused
            << " layoutDirty=" << stats.layout_dirty << " paintDirty=" << stats.paint_dirty << '\n';
    };

    measure("stable",false);
    measure("reordered",true);
}

} /* End of anonymous namespace */

int main()
{
    std::cout << "Reconciler benchmark (reconciliation only; excludes UI build/layout/paint)\n";
    for (int count : {100,1000,10000}) benchmark(count);
}
