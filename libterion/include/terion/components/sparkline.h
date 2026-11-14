/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#ifndef TERION_COMPONENTS_SPARKLINE_H
#define TERION_COMPONENTS_SPARKLINE_H

#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <terion/components/virtual_list.h>

namespace terion {

class Sparkline : public Component
{
public:
    enum class Variant { Blocks, Ascii };

    struct Props
    {
        std::vector<double> values = {};
        std::string key = {};
        LayoutStyle layout = {.width = 32,.height = 1};
        ColorVariant color = ColorVariant::Primary;
        Variant variant = Variant::Blocks;
        bool show_scrollbar = false;
    }; /* End of struct Props */

    explicit Sparkline(Props props) : Component(create(std::move(props))) {}

    [[nodiscard]] static std::string render(const std::vector<double>& values,int width,
                                            Variant variant = Variant::Blocks)
    {
        if (width <= 0) return {};
        constexpr const char* blocks[] = {"▁","▂","▃","▄","▅","▆","▇","█"};
        constexpr char ascii[] = ".:-=+*#@";
        const auto count = static_cast<std::size_t>(width);
        std::string output;
        if (values.empty()) return output;
        double low = std::numeric_limits<double>::infinity();
        double high = -std::numeric_limits<double>::infinity();
        for (const auto value : values)
            if (std::isfinite(value)) { low = std::min(low,value); high = std::max(high,value); }
        if (!std::isfinite(low)) return std::string(count,' ');
        for (std::size_t x = 0; x < count; ++x)
        {
            // Downsample large series by peak magnitude; interpolate when upsampling.
            const double start = static_cast<double>(x)*static_cast<double>(values.size())/static_cast<double>(count);
            const double end = static_cast<double>(x+1)*static_cast<double>(values.size())/static_cast<double>(count);
            double value = 0;
            bool valid = false;
            if (values.size() >= count)
            {
                const auto from = std::min(values.size()-1,static_cast<std::size_t>(start));
                const auto to = std::min(values.size(),std::max(from+1,static_cast<std::size_t>(std::ceil(end))));
                for (auto i = from; i < to; ++i)
                    if (std::isfinite(values[i]) && (!valid || values[i] > value))
                    { value = values[i]; valid = true; }
            }
            else
            {
                const double position = count == 1 ? 0.0 :
                    static_cast<double>(x)*static_cast<double>(values.size()-1)/static_cast<double>(count-1);
                const auto left = static_cast<std::size_t>(position);
                const auto right = std::min(left+1,values.size()-1);
                if (std::isfinite(values[left]) && std::isfinite(values[right]))
                {
                    value = values[left]+(values[right]-values[left])*(position-static_cast<double>(left));
                    valid = true;
                }
            }
            if (!valid) { output.push_back(' '); continue; }
            const double relative = high > low ? std::clamp((value-low)/(high-low),0.0,1.0) : 0.5;
            const auto index = static_cast<std::size_t>(std::lround(relative*7));
            if (variant == Variant::Blocks) output += blocks[index];
            else output.push_back(ascii[index]);
        }
        return output;
    }

private:
    static Component create(Props props)
    {
        const int width = std::max(0,props.layout.width.value_or(32));
        auto values = std::make_shared<std::vector<double>>(std::move(props.values));
        auto row = std::make_shared<std::string>(render(*values,width,props.variant));
        const auto color = props.color;
        return VirtualList({.item_count = 1,
            .item_styled = [row,color](int) -> StyledText { return {{.text = *row,.color = color}}; },
            .on_width_change = [values,row,variant = props.variant](int columns,int offset) {
                *row = render(*values,columns,variant);
                return std::pair<int,int>{1,offset};
            },.key = std::move(props.key),.layout = props.layout,
            .show_scrollbar = props.show_scrollbar});
    }
}; /* End of class Sparkline */

} /* End of namespace terion */

#endif /* TERION_COMPONENTS_SPARKLINE_H */
