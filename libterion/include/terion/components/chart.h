/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#ifndef TERION_COMPONENTS_CHART_H
#define TERION_COMPONENTS_CHART_H

#include <cstddef>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include <terion/components/virtual_list.h>

namespace terion {

class Chart : public Component
{
public:
    enum class Variant { Line, Scatter, Bars };

    struct Series
    {
        std::string name;
        std::vector<double> values;
        ColorVariant color = ColorVariant::Primary;
        bool operator==(const Series&) const = default;
    };

    // Horizontal thresholds are drawn in shared-Y mode only.
    struct ReferenceLine
    {
        double value = 0;
        std::string label = {};
        ColorVariant color = ColorVariant::Warning;
    };

    struct Bounds
    {
        double min = 0;
        double max = 1;
    };

    struct Props
    {
        std::vector<double> values = {};
        std::vector<Series> series = {};
        bool show_x_axis = false;
        bool show_y_axis = false;
        bool show_legend = false;
        bool auto_scale = true;
        double y_min = 0;
        double y_max = 1;
        std::function<void(std::size_t)> on_point_click = {};
        std::string key = {};
        LayoutStyle layout = {.width = 48,.height = 12};
        ColorVariant color = ColorVariant::Primary;
        Variant variant = Variant::Line;
        bool show_scrollbar = false;
        // Normalize each series to its own Y domain; labels use % of that domain.
        bool independent_y_axes = false;
        std::size_t sample_offset = 0;
        bool zero_baseline = false;
        // Sample index window. 0 count displays all available samples.
        std::size_t view_begin = 0;
        std::size_t view_count = 0;
        // Global sample index, shown as crosshair and multi-series readout.
        std::optional<std::size_t> cursor = std::nullopt;
        bool show_tooltip = false;
        std::function<void(std::size_t,std::size_t)> on_view_change = {};
        std::vector<ReferenceLine> reference_lines = {};
        bool show_grid = false;
        std::string y_unit = {};
    }; /* End of struct Props */

    explicit Chart(Props props) : Component(create(std::move(props))) {}

    [[nodiscard]] static std::vector<std::string> render(const std::vector<double>& values,
        int columns,int rows,Variant variant = Variant::Line,bool zero_baseline = false);

    // Styled rows retain series colors. The plot uses Braille cells (2x4 pixels).
    [[nodiscard]] static std::vector<StyledText> renderAdvanced(const std::vector<Series>& series,
        int width,int height,Variant variant = Variant::Line,bool show_x_axis = true,
        bool show_y_axis = true,bool show_legend = true,bool zero_baseline = false,
        bool auto_scale = true,double y_min = 0,double y_max = 1,
        std::size_t start_index = 0,bool independent_y_axes = false,
        const std::vector<ReferenceLine>& reference_lines = {},bool show_grid = false,
        std::string_view y_unit = {});


    // Reduction helper for external data providers. Retains original sample indices,
    // including spikes and gaps, so callers can map back to original timestamps.
    struct SamplePoint
    {
        std::size_t index;
        double value;
    };

    [[nodiscard]] static std::vector<SamplePoint> minMaxSample(
        const std::vector<double>& values,std::size_t buckets);

    [[nodiscard]] static std::size_t sampleCount(const std::vector<Series>& series,
        const std::vector<double>& values = {});

    [[nodiscard]] static std::pair<std::size_t,std::size_t> clampView(
        std::size_t total,std::size_t begin,std::size_t count);

    // Independent of terminal rendering; useful for streaming and unit tests.
    [[nodiscard]] static std::vector<Series> window(const std::vector<Series>& series,
        std::size_t begin,std::size_t count);

    [[nodiscard]] static std::string tooltip(const std::vector<Series>& series,
        std::size_t index,std::size_t sample_offset = 0);

    // Mouse-wheel or toolbar view helpers: magnify around the current center.
    [[nodiscard]] static std::pair<std::size_t,std::size_t> zoom(
        std::size_t total,std::size_t begin,std::size_t count,int direction);

    [[nodiscard]] static std::pair<std::size_t,std::size_t> pan(
        std::size_t total,std::size_t begin,std::size_t count,int direction);

private:
    struct Model
    {
        std::vector<double> values;
        std::vector<Series> series;
        std::vector<std::string> lines;
        std::vector<StyledText> styled_lines;
        bool advanced = false;
        bool independent_y_axes = false;
        std::size_t sample_offset = 0;
        bool show_x_axis = false;
        bool show_y_axis = false;
        bool show_legend = false;
        bool auto_scale = true;
        double y_min = 0;
        double y_max = 1;
        std::function<void(std::size_t)> on_point_click;
        Variant variant = Variant::Line;
        int height = 12;
        int layout_width = 48;
        bool zero_baseline = false;
        std::size_t view_begin = 0;
        std::size_t view_count = 0;
        std::optional<std::size_t> cursor;
        bool show_tooltip = false;
        std::function<void(std::size_t,std::size_t)> on_view_change;
        std::vector<ReferenceLine> reference_lines;
        bool show_grid = false;
        std::string y_unit;
    };

    static Component create(Props props);
}; /* End of class Chart */

} /* End of namespace terion */

#endif /* TERION_COMPONENTS_CHART_H */
