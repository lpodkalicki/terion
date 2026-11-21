/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <terion/components/chart.h>
#include <algorithm>
#include <cstddef>
#include <cmath>
#include <limits>
#include <functional>
#include <iomanip>
#include <sstream>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include <terion/components/virtual_list.h>
#include <terion/render/canvas.h>

namespace terion {

std::vector<std::string> Chart::render(const std::vector<double>& values,
        int columns,int rows,Variant variant,bool zero_baseline)
{
        if (columns <= 0 || rows <= 0) return {};
        render::Canvas canvas(columns*2,rows*4);
        if (values.empty()) return canvas.rows();
        double low = std::numeric_limits<double>::infinity();
        double high = -std::numeric_limits<double>::infinity();
        for (const auto value : values)
            if (std::isfinite(value)) { low = std::min(low,value); high = std::max(high,value); }
        if (!std::isfinite(low)) return canvas.rows();
        if (zero_baseline) { low = std::min(0.0,low); high = std::max(0.0,high); }
        if (low == high) { low -= 1.0; high += 1.0; }
        const int pixel_width = columns*2;
        const int pixel_height = rows*4;
        const auto y_of = [=](double value) {
            return std::clamp(static_cast<int>(std::lround((high-value)/(high-low)*(pixel_height-1))),0,pixel_height-1);
        };
        if (variant == Variant::Bars)
        {
            const int baseline = y_of(std::clamp(0.0,low,high));
            for (int x = 0; x < pixel_width; ++x)
            {
                const auto index = std::min(values.size()-1,
                    static_cast<std::size_t>(static_cast<double>(x)*values.size()/pixel_width));
                if (!std::isfinite(values[index])) continue;
                const int y = y_of(values[index]);
                canvas.line({x,std::min(y,baseline)},{x,std::max(y,baseline)});
            }
        }
        else
        {
            bool previous_valid = false;
            Point previous{};
            for (std::size_t index = 0; index < values.size(); ++index)
            {
                const auto value = values[index];
                if (!std::isfinite(value)) { previous_valid = false; continue; }
                const int x = values.size() == 1 ? 0 :
                    static_cast<int>(std::lround(static_cast<double>(index)*(pixel_width-1)/(values.size()-1)));
                const Point current{x,y_of(value)};
                if (variant == Variant::Line && previous_valid) canvas.line(previous,current);
                else canvas.point(current.x,current.y);
                previous = current;
                previous_valid = true;
            }
        }
        return canvas.rows();
    }

std::vector<StyledText> Chart::renderAdvanced(const std::vector<Series>& series,
        int width,int height,Variant variant,bool show_x_axis,
        bool show_y_axis,bool show_legend,bool zero_baseline,
        bool auto_scale,double y_min,double y_max,
        std::size_t start_index,bool independent_y_axes,
        const std::vector<ReferenceLine>& reference_lines,bool show_grid,
        std::string_view y_unit)
{
        if (width <= 0 || height <= 0) return {};
        const int margin = show_y_axis && width >= 14 ? 9 : 0;
        const int plot_width = std::max(1,width-margin);
        const int legend_rows = show_legend && !series.empty() && height >= 4 ? 1 : 0;
        const int x_rows = show_x_axis && height >= 3 ? 1 : 0;
        const int plot_height = std::max(1,height-legend_rows-x_rows);
        Bounds bounds{y_min,y_max};
        if (auto_scale)
        {
            bounds.min = std::numeric_limits<double>::infinity();
            bounds.max = -std::numeric_limits<double>::infinity();
            for (const auto& item : series)
                for (const double value : item.values)
                    if (std::isfinite(value))
                    {
                        bounds.min = std::min(bounds.min,value);
                        bounds.max = std::max(bounds.max,value);
                    }
            if (!std::isfinite(bounds.min)) bounds = {0,1};
        }
        if (zero_baseline) { bounds.min = std::min(bounds.min,0.0); bounds.max = std::max(bounds.max,0.0); }
        if (!(bounds.max > bounds.min)) { bounds.min -= 1.0; bounds.max += 1.0; }
        std::vector<Bounds> per_series;
        per_series.reserve(series.size());
        for (const auto& item : series)
        {
            Bounds own{std::numeric_limits<double>::infinity(),
                -std::numeric_limits<double>::infinity()};
            for (const double value : item.values)
                if (std::isfinite(value))
                {
                    own.min=std::min(own.min,value);
                    own.max=std::max(own.max,value);
                }
            if (!std::isfinite(own.min)) own={0,1};
            if (zero_baseline) { own.min=std::min(own.min,0.0); own.max=std::max(own.max,0.0); }
            if (!(own.max>own.min)) { own.min-=1; own.max+=1; }
            per_series.push_back(own);
        }
        const int pixel_width = plot_width*2;
        const int pixel_height = plot_height*4;
        const auto y_of = [&](double value,const Bounds& domain) {
            const double normalized = (domain.max-value)/(domain.max-domain.min);
            return std::clamp(static_cast<int>(std::lround(normalized*(pixel_height-1))),0,pixel_height-1);
        };
        // Separate canvases preserve each data series' color, even when lines cross.
        std::vector<std::vector<std::string>> painted;
        painted.reserve(series.size());
        std::size_t longest = 0;
        for (std::size_t series_index=0;series_index<series.size();++series_index)
        {
            const auto& item = series[series_index];
            const auto domain = independent_y_axes ? per_series[series_index] : bounds;
            render::Canvas canvas(pixel_width,pixel_height);
            const auto& values = item.values;
            longest = std::max(longest,values.size());
            if (!values.empty())
            {
                if (variant == Variant::Bars)
                {
                    const int baseline = y_of(std::clamp(0.0,domain.min,domain.max),domain);
                    for (int x = 0; x < pixel_width; ++x)
                    {
                        const auto index = std::min(values.size()-1,
                            static_cast<std::size_t>(static_cast<double>(x)*values.size()/pixel_width));
                        if (!std::isfinite(values[index])) continue;
                        const int y = y_of(values[index],domain);
                        canvas.line({x,std::min(y,baseline)},{x,std::max(y,baseline)});
                    }
                }
                else
                {
                    bool previous_valid = false;
                    Point previous{};
                    for (std::size_t index = 0; index < values.size(); ++index)
                    {
                        if (!std::isfinite(values[index])) { previous_valid = false; continue; }
                        const int x = values.size() == 1 ? 0 :
                            static_cast<int>(std::lround(static_cast<double>(index)*(pixel_width-1)/(values.size()-1)));
                        const Point current{x,y_of(values[index],domain)};
                        if (variant == Variant::Line && previous_valid) canvas.line(previous,current);
                        else canvas.point(current.x,current.y);
                        previous = current;
                        previous_valid = true;
                    }
                }
            }
            painted.push_back(canvas.rows());
        }
        // Draw reference overlays on separate canvases. Series pixels take precedence.
        // Values are meaningful only on the common Y domain.
        render::Canvas grid(pixel_width,pixel_height);
        std::vector<std::vector<std::string>> thresholds;
        if (show_grid)
            for (int level = 1; level < 4; ++level)
            {
                const int y = (pixel_height-1)*level/4;
                for (int x = 0; x < pixel_width; x += 4) grid.point(x,y);
            }
        if (!independent_y_axes)
            for (const auto& reference : reference_lines)
            {
                render::Canvas threshold(pixel_width,pixel_height);
                if (std::isfinite(reference.value) && reference.value >= bounds.min &&
                    reference.value <= bounds.max)
                {
                    const int y = y_of(reference.value,bounds);
                    for (int x = 0; x < pixel_width; x += 2) threshold.point(x,y);
                }
                thresholds.push_back(threshold.rows());
            }
        const auto grid_rows = grid.rows();
        std::vector<StyledText> output;
        output.reserve(static_cast<std::size_t>(height));
        for (int y = 0; y < plot_height; ++y)
        {
            StyledText row;
            if (margin)
            {
                std::ostringstream label;
                label << std::fixed << std::setprecision(independent_y_axes ? 0 : 2)
                    << (independent_y_axes ? 100.0*(plot_height-1-y)/std::max(1,plot_height-1) :
                    bounds.max-(bounds.max-bounds.min)*y/std::max(1,plot_height-1));
                auto display = label.str();
                display += y_unit;
                if (display.size() > 7) display.resize(7);
                row.push_back({std::string(7-display.size(),' ')+display+" │",ColorVariant::Info});
            }
            for (int x = 0; x < plot_width; ++x)
            {
                std::string glyph = " ";
                ColorVariant color = ColorVariant::Default;
                for (std::size_t k = 0; k < series.size(); ++k)
                {
                    const auto& encoded = painted[k][static_cast<std::size_t>(y)];
                    const auto offset = static_cast<std::size_t>(x)*3;
                    if (offset+3 > encoded.size()) continue;
                    const auto current = encoded.substr(offset,3);
                    if (current != "\xE2\xA0\x80") { glyph = current; color = series[k].color; }
                }
                if (glyph == " " && !independent_y_axes)
                    for (std::size_t k = 0; k < thresholds.size(); ++k)
                    {
                        const auto& encoded = thresholds[k][static_cast<std::size_t>(y)];
                        const auto offset = static_cast<std::size_t>(x)*3;
                        if (offset+3 > encoded.size()) continue;
                        const auto current = encoded.substr(offset,3);
                        if (current != "\xE2\xA0\x80")
                        {
                            glyph = current;
                            color = reference_lines[k].color;
                            break;
                        }
                    }
                if (glyph == " " && show_grid)
                {
                    const auto& encoded = grid_rows[static_cast<std::size_t>(y)];
                    const auto offset = static_cast<std::size_t>(x)*3;
                    if (offset+3 <= encoded.size())
                    {
                        const auto current = encoded.substr(offset,3);
                        if (current != "\xE2\xA0\x80")
                        {
                            glyph = current;
                            color = ColorVariant::Info;
                        }
                    }
                }
                if (!row.empty() && row.back().color == color) row.back().text += glyph;
                else row.push_back({glyph,color});
            }
            output.push_back(std::move(row));
        }
        if (x_rows)
        {
            StyledText row;
            if (margin) row.push_back({"        └",ColorVariant::Info});
            std::string ticks(static_cast<std::size_t>(plot_width),'-');
            const auto last = longest > 0 ? std::to_string(start_index+longest-1) : std::string("0");
            const auto first = std::to_string(start_index);
            if (first.size() <= ticks.size()) ticks.replace(0,first.size(),first);
            if (last.size() < ticks.size())
                ticks.replace(ticks.size()-last.size(),last.size(),last);
            row.push_back({ticks,ColorVariant::Info});
            output.push_back(std::move(row));
        }
        if (legend_rows)
        {
            StyledText row;
            for (const auto& item : series)
            {
                const std::string label = " ● " + item.name;
                std::size_t occupied = 0;
                for (const auto& span : row) occupied += span.text.size();
                if (occupied+label.size() > static_cast<std::size_t>(width)) break;
                row.push_back({label,item.color,true});
            }
            if (!independent_y_axes)
                for (const auto& reference : reference_lines)
                {
                    if (reference.label.empty()) continue;
                    const std::string label = " ! " + reference.label;
                    std::size_t occupied = 0;
                    for (const auto& span : row) occupied += span.text.size();
                    if (occupied+label.size() > static_cast<std::size_t>(width)) break;
                    row.push_back({label,reference.color,true});
                }
            output.push_back(std::move(row));
        }
        return output;
    }

std::vector<Chart::SamplePoint> Chart::minMaxSample(
        const std::vector<double>& values,std::size_t buckets)
{
        std::vector<SamplePoint> result;
        if (values.empty() || buckets == 0) return result;
        const auto sections = std::min(buckets,values.size());
        result.reserve(sections*2);
        for (std::size_t bucket=0; bucket<sections; ++bucket)
        {
            const auto begin = bucket*values.size()/sections;
            const auto end = (bucket+1)*values.size()/sections;
            std::size_t minimum=values.size(),maximum=values.size();
            for (std::size_t i=begin;i<end;++i)
            {
                if (!std::isfinite(values[i])) continue;
                if (minimum==values.size() || values[i]<values[minimum]) minimum=i;
                if (maximum==values.size() || values[i]>values[maximum]) maximum=i;
            }
            if (minimum==values.size()) continue;
            if (minimum>maximum) std::swap(minimum,maximum);
            result.push_back({minimum,values[minimum]});
            if (maximum!=minimum) result.push_back({maximum,values[maximum]});
        }
        return result;
    }

std::size_t Chart::sampleCount(const std::vector<Series>& series,
        const std::vector<double>& values)
{
        std::size_t count = values.size();
        for (const auto& item : series) count = std::max(count,item.values.size());
        return count;
    }

std::pair<std::size_t,std::size_t> Chart::clampView(
        std::size_t total,std::size_t begin,std::size_t count)
{
        if (!total) return {0,0};
        begin = std::min(begin,total-1);
        if (!count) count = total-begin;
        count = std::clamp(count,std::size_t{1},total);
        begin = std::min(begin,total-count);
        return {begin,count};
    }

std::vector<Chart::Series> Chart::window(const std::vector<Series>& series,
        std::size_t begin,std::size_t count)
{
        std::vector<Series> result;
        result.reserve(series.size());
        for (const auto& item : series)
        {
            Series cropped{item.name,{},item.color};
            if (begin < item.values.size())
            {
                const auto end = begin+std::min(count,item.values.size()-begin);
                cropped.values.assign(item.values.begin()+static_cast<std::ptrdiff_t>(begin),
                    item.values.begin()+static_cast<std::ptrdiff_t>(end));
            }
            result.push_back(std::move(cropped));
        }
        return result;
    }

std::string Chart::tooltip(const std::vector<Series>& series,
        std::size_t index,std::size_t sample_offset)
{
        std::ostringstream output;
        output << "x=" << sample_offset+index;
        for (const auto& item : series)
        {
            output << "  " << item.name << '=';
            if (index < item.values.size() && std::isfinite(item.values[index]))
                output << std::fixed << std::setprecision(2) << item.values[index];
            else output << "n/a";
        }
        return output.str();
    }

std::pair<std::size_t,std::size_t> Chart::zoom(
        std::size_t total,std::size_t begin,std::size_t count,int direction)
{
        const auto current = clampView(total,begin,count);
        if (!current.second) return current;
        const auto center = current.first+current.second/2;
        std::size_t next = current.second;
        if (direction > 0) next = std::max(std::size_t{2},current.second/2);
        if (direction < 0) next = std::min(total,current.second > total/2 ? total : current.second*2);
        next = std::clamp(next,std::size_t{1},total);
        const auto next_begin = center >= next/2 ? center-next/2 : 0;
        return clampView(total,next_begin,next);
    }

std::pair<std::size_t,std::size_t> Chart::pan(
        std::size_t total,std::size_t begin,std::size_t count,int direction)
{
        const auto current = clampView(total,begin,count);
        if (!current.second) return current;
        const auto step = std::max(std::size_t{1},current.second/5);
        if (direction < 0) return {current.first > step ? current.first-step : 0,current.second};
        return {std::min(total-current.second,current.first+step),current.second};
    }

Component Chart::create(Props props)
{
        auto model = std::make_shared<Model>();
        model->values = std::move(props.values);
        model->series = std::move(props.series);
        model->advanced = !model->series.empty();
        model->independent_y_axes = props.independent_y_axes;
        model->sample_offset = props.sample_offset;
        model->variant = props.variant;
        model->height = std::max(1,props.layout.height.value_or(12));
        model->zero_baseline = props.zero_baseline;
        model->view_begin = props.view_begin;
        model->view_count = props.view_count;
        model->cursor = props.cursor;
        model->show_tooltip = props.show_tooltip;
        model->on_view_change = std::move(props.on_view_change);
        model->reference_lines = std::move(props.reference_lines);
        model->show_grid = props.show_grid;
        model->y_unit = std::move(props.y_unit);
        model->show_x_axis = props.show_x_axis;
        model->show_y_axis = props.show_y_axis;
        model->show_legend = props.show_legend;
        model->auto_scale = props.auto_scale;
        model->y_min = props.y_min;
        model->y_max = props.y_max;
        model->on_point_click = std::move(props.on_point_click);
        model->layout_width = std::max(1,props.layout.width.value_or(48));
        const auto color = props.color;
        const auto reflow = [model](int width) {
            model->layout_width = width;
            const auto total = sampleCount(model->series,model->values);
            const auto view = clampView(total,model->view_begin,model->view_count);
            const auto visible_series = window(model->series,view.first,view.second);
            if (model->advanced)
            {
                const int plot_height = model->height-(model->show_tooltip ? 1 : 0);
                model->styled_lines = renderAdvanced(visible_series,width,plot_height,model->variant,
                    model->show_x_axis,model->show_y_axis,model->show_legend,model->zero_baseline,
                    model->auto_scale,model->y_min,model->y_max,view.first+model->sample_offset,
                    model->independent_y_axes,model->reference_lines,model->show_grid,model->y_unit);
                if (model->cursor && view.second && *model->cursor >= view.first &&
                    *model->cursor-view.first < view.second)
                {
                    const int margin = model->show_y_axis && width >= 14 ? 9 : 0;
                    const int plot_width = std::max(1,width-margin);
                    const auto relative = *model->cursor-view.first;
                    const int column = margin+static_cast<int>(std::lround(
                        static_cast<double>(relative)*(plot_width-1)/std::max(std::size_t{1},view.second-1)));
                    const int reserved = (model->show_x_axis && plot_height >= 3 ? 1 : 0)+
                        (model->show_legend && !model->series.empty() && plot_height >= 4 ? 1 : 0);
                    for (int y = 0; y < plot_height-reserved; ++y)
                    {
                        auto& spans = model->styled_lines[static_cast<std::size_t>(y)];
                        StyledText highlighted;
                        int x = 0;
                        for (const auto& span : spans)
                        {
                            for (std::size_t pos = 0; pos < span.text.size();)
                            {
                                const unsigned char first = static_cast<unsigned char>(span.text[pos]);
                                const std::size_t length = first < 0x80 ? 1 :
                                    (first < 0xE0 ? 2 : (first < 0xF0 ? 3 : 4));
                                const auto glyph = span.text.substr(pos,std::min(length,span.text.size()-pos));
                                const auto color = x == column ? ColorVariant::Warning : span.color;
                                if (!highlighted.empty() && highlighted.back().color == color &&
                                    highlighted.back().bold == (x == column || span.bold))
                                    highlighted.back().text += glyph;
                                else highlighted.push_back({glyph,color,x == column || span.bold});
                                pos += length;
                                ++x;
                            }
                        }
                        spans = std::move(highlighted);
                    }
                }
                if (model->show_tooltip)
                {
                    const auto label = model->cursor ? tooltip(model->series,*model->cursor,model->sample_offset) :
                        std::string("Click a sample to inspect | wheel: zoom | Shift+wheel: pan");
                    model->styled_lines.push_back({{label.substr(0,static_cast<std::size_t>(width)),ColorVariant::Info}});
                }
            }
            else
            {
                std::vector<double> cropped;
                if (view.first < model->values.size())
                {
                    const auto end = view.first+std::min(view.second,model->values.size()-view.first);
                    cropped.assign(model->values.begin()+static_cast<std::ptrdiff_t>(view.first),
                        model->values.begin()+static_cast<std::ptrdiff_t>(end));
                }
                model->lines = render(cropped,width,model->height,model->variant,model->zero_baseline);
            }
        };
        reflow(std::max(1,props.layout.width.value_or(48)));
        return VirtualList({.item_count = model->height,
            .item_styled = [model,color](int index) -> StyledText {
                if (model->advanced) return model->styled_lines.at(static_cast<std::size_t>(index));
                return {{.text = model->lines.at(static_cast<std::size_t>(index)),.color = color}};
            },.on_width_change = [model,reflow](int columns,int offset) {
                reflow(std::max(1,columns));
                return std::pair<int,int>{model->height,offset};
            },.on_click_position = [model](int column,int row) {
                const int reserved = (model->advanced && model->show_x_axis && model->height >= 3 ? 1 : 0)+
                    (model->advanced && model->show_legend && !model->series.empty() && model->height >= 4 ? 1 : 0);
                if (row >= model->height-reserved-(model->show_tooltip ? 1 : 0)) return;
                if (!model->on_point_click) return;
                const auto view = clampView(sampleCount(model->series,model->values),
                    model->view_begin,model->view_count);
                if (!view.second) return;
                const int margin = model->advanced && model->show_y_axis && model->layout_width >= 14 ? 9 : 0;
                const int usable = std::max(1,model->layout_width-margin-1);
                const int relative = std::clamp(column-margin,0,usable);
                const auto index = static_cast<std::size_t>(std::lround(
                    static_cast<double>(relative)*(view.second-1)/usable));
                model->on_point_click(view.first+index);
            },.key = std::move(props.key),.id = std::move(props.id),.layout = props.layout,
            .show_scrollbar = props.show_scrollbar,
            .on_event = [model](ComponentEvent& event) {
                if (!model->on_view_change || event.event.key != Key::Mouse ||
                    event.event.mouse.action != MouseAction::Scroll) return;
                const int delta = event.event.mouse.scroll_y;
                if (!delta) return;
                const auto total = sampleCount(model->series,model->values);
                const auto view = event.event.mouse.shift ?
                    pan(total,model->view_begin,model->view_count,delta > 0 ? -1 : 1) :
                    zoom(total,model->view_begin,model->view_count,delta > 0 ? 1 : -1);
                model->on_view_change(view.first,view.second);
                event.preventDefault();
            }});
    }

} /* End of namespace terion */
