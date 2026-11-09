/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <terion/render_node.h>
#include <terion/utf8.h>
#include <algorithm>
#include <cmath>
#include <limits>

using namespace terion;

bool RenderNode::hasIndeterminateProgress() const noexcept
{
    if (type_ == Component::Type::TabPanel && !tab_panel_active_) return false;
    if (type_ == Component::Type::Progress && progress_indeterminate_ &&
        bounds_.width > 0 && bounds_.height > 0) return true;
    for (const auto& child : children_)
        if (child->hasIndeterminateProgress()) return true;
    return false;
}

namespace {

int addBounded(int a, int b);

int textWidth(const std::string_view text)
{
    int width = 0;
    for (std::size_t i = 0; i < text.size();)
    {
        const auto [code_point,bytes] = decodeUtf8(text,i);
        if (!bytes) break;
        width = addBounded(width,codePointWidth(code_point));
        i += bytes;
    }
    return width;
}

std::string clipText(const std::string_view text, const int columns)
{
    std::string clipped;
    int width = 0;
    for (std::size_t i = 0; i < text.size();)
    {
        const auto [code_point,bytes] = decodeUtf8(text,i);
        if (!bytes) break;
        const int current = codePointWidth(code_point);
        if (current > columns - width) break;
        if (current > 0 || !clipped.empty()) clipped.append(text.substr(i,bytes));
        width += current;
        i += bytes;
    }
    return clipped;
}

std::string clipWindow(const std::string_view text, const int offset, const int columns)
{
    std::string result;
    int position = 0;
    int drawn = 0;
    for (std::size_t i = 0; i < text.size();)
    {
        const auto [code_point,bytes] = decodeUtf8(text,i);
        if (!bytes) break;
        const int width = codePointWidth(code_point);
        if (position >= offset && drawn + width <= columns)
        {
            result.append(text.substr(i,bytes));
            drawn += width;
        }
        if (width > 0) position += width;
        i += bytes;
        if (drawn >= columns) break;
    }
    return result;
}

int addBounded(const int a, const int b)
{
    return static_cast<int>(std::min(static_cast<long long>(a) + b,
        static_cast<long long>(std::numeric_limits<int>::max())));
}

Color blendColor(const Color base, const Color overlay, const int overlay_percent)
{
    const auto mix = [&](const std::uint8_t a, const std::uint8_t b) {
        return static_cast<std::uint8_t>((static_cast<int>(a) * (100 - overlay_percent) +
            static_cast<int>(b) * overlay_percent) / 100);
    };
    return Color::rgb(mix(base.red,overlay.red),mix(base.green,overlay.green),mix(base.blue,overlay.blue));
}

enum class ScrollbarInteraction { Normal, Hover, Active };

void paintScrollbar(Painter& painter, const Rect track, const Rect thumb, const Theme& theme,
    const Color background, const bool unicode,
    const ScrollbarInteraction interaction = ScrollbarInteraction::Normal)
{
    if (track.width <= 0 || track.height <= 0) return;
    const auto& scrollbar = theme.scrollbar;
    for (int y = track.y; y < track.y+track.height; ++y)
    {
        const bool selected = y >= thumb.y && y < thumb.y+thumb.height;
        const TextStyle style {.foreground = selected ?
            (interaction == ScrollbarInteraction::Active ? scrollbar.active_color :
             interaction == ScrollbarInteraction::Hover ? scrollbar.hover_color : scrollbar.thumb_color) :
            scrollbar.track_color,
            .background = background};
        painter.put(track.x,y,selected ? (unicode ? theme.scrollbar.thumb : "#") :
            (unicode ? theme.scrollbar.track : "|"),style);
    }
}

} /* End of anonymous namespace */

Size RenderNode::measure(const Constraints& constraints) const
{
    if (type_ == Component::Type::TabPanel && !tab_panel_active_) return {};
    const int padding = std::max(0,style_.padding) + (border_ ? 1 : 0);
    const int gap = std::max(0,style_.gap);
    Size content{};

    if (type_ == Component::Type::Divider)
    {
        content = {.width = constraints.maximum.width,.height = 1};
    }
    else if (type_ == Component::Type::Dialog || type_ == Component::Type::Toast)
    {
        content = {};
    }
    else if (type_ == Component::Type::Progress || type_ == Component::Type::Slider)
    {
        content = {.width = 20,.height = 1};
    }
    else if (type_ == Component::Type::ResizableSplit)
    {
        if (children_.size() == 2)
        {
            const Size a = children_[0]->measure(constraints);
            const Size b = children_[1]->measure(constraints);
            content = split_direction_ == SplitDirection::Horizontal ?
                Size{.width = addBounded(addBounded(a.width,b.width),1),.height = std::max(a.height,b.height)} :
                Size{.width = std::max(a.width,b.width),.height = addBounded(addBounded(a.height,b.height),1)};
        }
    }
    else if (type_ == Component::Type::Button)
    {
        const int padding = button_size_ == ButtonSize::Large ? 3 : button_size_ == ButtonSize::Compact ? 1 : 2;
        content = {.width = addBounded(textWidth(text_),padding * 2 +
            (button_variant_ == ButtonVariant::Outlined ? 2 : 0)),
            .height = button_size_ == ButtonSize::Compact ? 1 : button_size_ == ButtonSize::Large ? 5 : 3};
    }
    else if (type_ == Component::Type::Checkbox || type_ == Component::Type::Switch ||
        type_ == Component::Type::Radio)
    {
        content = {.width = addBounded(textWidth(text_),type_ == Component::Type::Switch ? 7 : 5),.height = 1};
    }
    else if (type_ == Component::Type::DropdownMenu)
    {
        const int padding = button_size_ == ButtonSize::Large ? 3 : button_size_ == ButtonSize::Compact ? 1 : 2;
        content = {.width = addBounded(textWidth(text_),padding*2+2+
            (button_variant_ == ButtonVariant::Outlined ? 2 : 0)),
            .height = button_size_ == ButtonSize::Compact ? 1 : button_size_ == ButtonSize::Large ? 5 : 3};
    }
    else if (type_ == Component::Type::RadioGroup)
    {
        int width = 0;
        for (const auto& option : radio_options_) width = std::max(width,textWidth(option.label));
        content = {.width = addBounded(width,5),.height = static_cast<int>(radio_options_.size())};
    }
    else if (type_ == Component::Type::Tabs)
    {
        int width = 0;
        for (const auto& option : tab_options_) width = addBounded(width,textWidth(option.label)+4);
        content = {.width = width,.height = 3};
    }
    else if (type_ == Component::Type::Table)
    {
        int width = 1;
        for (std::size_t i = 0; i < table_columns_.size(); ++i)
        {
            int column_width = textWidth(table_columns_[i].title);
            for (const auto& row : table_rows_)
            {
                if (table_filter_ && !table_filter_(row)) continue;
                const auto cell = table_format_cell_ ? table_format_cell_(row,i) :
                    TableCellFormat{.text = i < row.cells.size() ? row.cells[i] : std::string{}};
                column_width = std::max(column_width,textWidth(cell.text));
            }
            width = addBounded(width,std::max(1,table_columns_[i].width > 0 ?
                table_columns_[i].width : column_width+2)+1);
        }
        content = {.width = width,.height = addBounded(static_cast<int>(tableOrder().size()),2)};
    }
    else if (type_ == Component::Type::TreeView)
    {
        const auto visible = treeVisibleItems();
        int width = 0;
        for (const auto& entry : visible)
            width = std::max(width,entry.depth*2+4+textWidth(entry.item->icon)+
                textWidth(entry.item->label));
        content = {.width = width,.height = static_cast<int>(visible.size())};
    }
    else if (type_ == Component::Type::TextField || type_ == Component::Type::Select)
    {
        const int padding = button_size_ == ButtonSize::Large ? 3 : button_size_ == ButtonSize::Compact ? 1 : 2;
        int content_width = std::max(textWidth(text_),textWidth(placeholder_));
        if (type_ == Component::Type::Select)
            for (const auto& option : select_options_) content_width = std::max(content_width,textWidth(option.label));
        content = {.width = addBounded(content_width,padding * 2 +
            (type_ == Component::Type::Select ? 3 : text_field_variant_ == TextFieldVariant::Outlined ? 2 : 0)),
            .height = button_size_ == ButtonSize::Compact ? 1 : button_size_ == ButtonSize::Large ? 5 : 3};
    }
    else if (type_ == Component::Type::Text)
    {
        content = {.width = textWidth(text_),.height = 1};
    }
    else if (type_ == Component::Type::ScrollView)
    {
        const int inset = addBounded(padding,padding);
        const int available_width = std::max(0,constraints.maximum.width - std::min(inset,constraints.maximum.width));
        const int gap_space = std::max(0,style_.gap);
        int width = 0;
        int height = 0;
        int visible = 0;
        for (std::size_t i = 0; i < children_.size(); ++i)
        {
            if (!children_[i]->tabPanelActive()) continue;
            const auto child = children_[i]->measure(Constraints{.maximum = {
                .width = available_width,.height = std::numeric_limits<int>::max() / 4}});
            width = std::max(width,child.width);
            if (visible++ > 0) height = addBounded(height,gap_space);
            height = addBounded(height,child.height);
        }
        content = {.width = addBounded(width,inset),.height = addBounded(height,inset)};
    }
    else if (type_ == Component::Type::Grid)
    {
        const int inset = addBounded(padding,padding);
        const int columns = std::max(1,columns_);
        const int inner_width = std::max(0,constraints.maximum.width - std::min(inset,constraints.maximum.width));
        const int available_width = std::max(0,inner_width - static_cast<int>(std::min(
            static_cast<long long>(inner_width),static_cast<long long>(columns - 1) * gap)));
        const int cell_width = available_width / columns;
        int largest_width = 0;
        int row_height = 0;
        int total_height = 0;
        for (std::size_t i = 0; i < children_.size(); ++i)
        {
            const auto child_size = children_[i]->measure(Constraints{.maximum = {
                .width = cell_width,.height = constraints.maximum.height}});
            largest_width = std::max(largest_width,child_size.width);
            row_height = std::max(row_height,child_size.height);
            if (i % static_cast<std::size_t>(columns) == static_cast<std::size_t>(columns - 1) ||
                i + 1 == children_.size())
            {
                total_height = addBounded(total_height,row_height);
                if (i + 1 < children_.size()) total_height = addBounded(total_height,gap);
                row_height = 0;
            }
        }
        content = {.width = static_cast<int>(std::min(static_cast<long long>(std::numeric_limits<int>::max()),
            static_cast<long long>(largest_width) * columns + static_cast<long long>(columns - 1) * gap + inset)),
            .height = addBounded(total_height,inset)};
    }
    else
    {
        const int inset = addBounded(padding,padding);
        const Size max_content {.width = std::max(0,constraints.maximum.width - std::min(inset,constraints.maximum.width)),
            .height = std::max(0,constraints.maximum.height - std::min(inset,constraints.maximum.height))};
        int visible_children = 0;
        for (std::size_t i = 0; i < children_.size(); ++i)
        {
            const auto& child = children_[i];
            if (child->type_ == Component::Type::Dialog || child->type_ == Component::Type::Toast ||
                !child->tabPanelActive()) continue;
            const auto child_size = child->measure(Constraints{.maximum = max_content});
            if (type_ == Component::Type::Row)
            {
                if (visible_children > 0) content.width = addBounded(content.width,gap);
                content.width = addBounded(content.width,child_size.width);
                content.height = std::max(content.height,child_size.height);
            }
            else
            {
                if (visible_children > 0) content.height = addBounded(content.height,gap);
                content.height = addBounded(content.height,child_size.height);
                content.width = std::max(content.width,child_size.width);
            }
            ++visible_children;
        }
        content.width = addBounded(content.width,inset);
        content.height = addBounded(content.height,inset);
    }

    if (style_.width) content.width = std::max(0,*style_.width);
    if (style_.height) content.height = std::max(0,*style_.height);
    if (style_.minWidth) content.width = std::max(content.width,std::max(0,*style_.minWidth));
    if (style_.maxWidth) content.width = std::min(content.width,std::max(0,*style_.maxWidth));
    if (style_.minHeight) content.height = std::max(content.height,std::max(0,*style_.minHeight));
    if (style_.maxHeight) content.height = std::min(content.height,std::max(0,*style_.maxHeight));
    return constraints.constrain(content);
}

void RenderNode::layout(const Constraints& constraints, const Point origin)
{
    const auto size = measure(constraints);
    bounds_ = {.x = origin.x,.y = origin.y,.width = size.width,.height = size.height};
    if (type_ == Component::Type::Table)
        table_scroll_.configure(static_cast<int>(tableOrder().size()),std::max(0,size.height-2));
    if (type_ == Component::Type::TreeView)
    {
        const auto visible = treeVisibleItems();
        tree_scroll_.configure(static_cast<int>(visible.size()),size.height);
        if (tree_pending_reveal_)
        {
            for (std::size_t i = 0; i < visible.size(); ++i)
            {
                if (visible[i].item->key != text_) continue;
                const int index = static_cast<int>(i);
                tree_scroll_.revealItem(index);
                break;
            }
            tree_pending_reveal_ = false;
        }
    }
    if (type_ == Component::Type::TabPanel && !tab_panel_active_) return;
    if (type_ == Component::Type::ResizableSplit && children_.size() == 2)
    {
        const bool horizontal = split_direction_ == SplitDirection::Horizontal;
        const int extent = horizontal ? size.width : size.height;
        const int available = std::max(0,extent-1);
        const int lower = std::min(split_min_first_,available);
        const int upper = std::max(lower,available-split_min_second_);
        const int first = std::clamp(split_position_,lower,upper);
        const Size first_size = horizontal ? Size{.width = first,.height = size.height} :
            Size{.width = size.width,.height = first};
        const Size second_size = horizontal ? Size{.width = available-first,.height = size.height} :
            Size{.width = size.width,.height = available-first};
        children_[0]->layout({.minimum = first_size,.maximum = first_size},origin);
        children_[1]->layout({.minimum = second_size,.maximum = second_size},
            horizontal ? Point{.x = origin.x+first+1,.y = origin.y} :
                Point{.x = origin.x,.y = origin.y+first+1});
        return;
    }
    if (type_ == Component::Type::Text || type_ == Component::Type::Dialog ||
        type_ == Component::Type::Toast || type_ == Component::Type::Button ||
        type_ == Component::Type::Checkbox || type_ == Component::Type::Switch ||
        type_ == Component::Type::Radio || type_ == Component::Type::RadioGroup ||
        type_ == Component::Type::Tabs || type_ == Component::Type::Table ||
        type_ == Component::Type::TreeView ||
        type_ == Component::Type::TextField || type_ == Component::Type::Select ||
        type_ == Component::Type::DropdownMenu || type_ == Component::Type::Divider ||
        type_ == Component::Type::Progress || type_ == Component::Type::Slider) return;

    const int padding = std::max(0,style_.padding) + (border_ ? 1 : 0);
    const int gap = std::max(0,style_.gap);
    const int inset = addBounded(padding,padding);
    const int inner_width = std::max(0,size.width - inset);
    const int inner_height = std::max(0,size.height - inset);
    if (type_ == Component::Type::ScrollView)
    {
        const int gap = std::max(0,style_.gap);
        std::vector<Size> sizes;
        const auto measure_content = [&](const int width) {
            sizes.clear();
            int height = 0;
            int visible = 0;
            for (std::size_t i = 0; i < children_.size(); ++i)
            {
                if (!children_[i]->tabPanelActive()) { sizes.push_back({}); continue; }
                const auto child_size = children_[i]->measure(Constraints{.maximum = {
                    .width = width,.height = std::numeric_limits<int>::max() / 4}});
                sizes.push_back(child_size);
                if (visible++ > 0) height = addBounded(height,gap);
                height = addBounded(height,child_size.height);
            }
            return height;
        };
        int content_height = measure_content(inner_width);
        if (scrollbar_visible_ && content_height > inner_height && inner_width > 0)
            content_height = measure_content(inner_width - 1);
        scroll_.configure(content_height,inner_height);
        const int offset = scroll_.offset();
        int y = origin.y + padding - offset;
        const int child_width = scrollbar_visible_ && scroll_.maximum() > 0 && inner_width > 0 ?
            inner_width - 1 : inner_width;
        int placed = 0;
        for (std::size_t i = 0; i < children_.size(); ++i)
        {
            if (!children_[i]->tabPanelActive())
            {
                children_[i]->layout(Constraints{.maximum = {}},{.x = origin.x,.y = origin.y});
                continue;
            }
            if (placed++ > 0) y += gap;
            children_[i]->layout(Constraints{.maximum = {.width = child_width,.height = sizes[i].height}},
                {.x = origin.x + padding,.y = y});
            y += sizes[i].height;
        }
        return;
    }
    if (type_ == Component::Type::Grid)
    {
        const int columns = std::max(1,columns_);
        const int available_width = std::max(0,inner_width - static_cast<int>(std::min(
            static_cast<long long>(inner_width),static_cast<long long>(columns - 1) * gap)));
        const int column_width = available_width / columns;
        const int remainder = available_width % columns;
        int y = origin.y + padding;
        for (std::size_t row = 0; row * static_cast<std::size_t>(columns) < children_.size(); ++row)
        {
            int row_height = 0;
            int x = origin.x + padding;
            for (int col = 0; col < columns; ++col)
            {
                const std::size_t index = row * static_cast<std::size_t>(columns) + static_cast<std::size_t>(col);
                if (index >= children_.size()) break;
                const int width = column_width + (col < remainder ? 1 : 0);
                const auto child_size = children_[index]->measure(Constraints{.maximum = {
                    .width = width,.height = std::max(0,inner_height - (y - origin.y - padding))}});
                row_height = std::max(row_height,child_size.height);
                x += width + gap;
            }
            x = origin.x + padding;
            for (int col = 0; col < columns; ++col)
            {
                const std::size_t index = row * static_cast<std::size_t>(columns) + static_cast<std::size_t>(col);
                if (index >= children_.size()) break;
                const int width = column_width + (col < remainder ? 1 : 0);
                children_[index]->layout(Constraints{.maximum = {.width = width,.height = row_height}},
                    {.x = x,.y = y});
                x += width + gap;
            }
            y += row_height + gap;
        }
        return;
    }
    if (type_ == Component::Type::Row || type_ == Component::Type::Column)
    {
        const bool horizontal = type_ == Component::Type::Row;
        const int main_extent = horizontal ? inner_width : inner_height;
        const int cross_extent = horizontal ? inner_height : inner_width;
        int visible_children = 0;
        for (const auto& child : children_)
            if (child->type_ != Component::Type::Dialog && child->tabPanelActive()) ++visible_children;
        const int gaps = std::max(0,visible_children-1);
        const int gap_space = static_cast<int>(std::min(static_cast<long long>(main_extent),
            static_cast<long long>(gaps) * gap));
        std::vector<int> sizes(children_.size(),0);
        int fixed_extent = 0;
        long long total_flex = 0;
        for (std::size_t i = 0; i < children_.size(); ++i)
        {
            if (children_[i]->type_ == Component::Type::Dialog || !children_[i]->tabPanelActive()) continue;
            const int weight = std::max(0,children_[i]->style_.flex);
            if (weight > 0)
                total_flex += weight;
            else
            {
                const auto natural = children_[i]->measure(Constraints{.maximum = {
                    .width = inner_width,.height = inner_height}});
                sizes[i] = std::min(horizontal ? natural.width : natural.height,
                    std::max(0,main_extent - gap_space - std::min(main_extent,fixed_extent)));
                fixed_extent = addBounded(fixed_extent,sizes[i]);
            }
        }
        const int available = std::max(0,main_extent - gap_space - std::min(main_extent,fixed_extent));
        long long remaining_weight = total_flex;
        int remaining_space = available;
        for (std::size_t i = 0; i < children_.size(); ++i)
        {
            if (children_[i]->type_ == Component::Type::Dialog || !children_[i]->tabPanelActive()) continue;
            const int weight = std::max(0,children_[i]->style_.flex);
            if (weight > 0)
            {
                sizes[i] = remaining_weight ? static_cast<int>(static_cast<long long>(remaining_space) *
                    weight / remaining_weight) : 0;
                remaining_space -= sizes[i];
                remaining_weight -= weight;
            }
        }
        int occupied = gap_space;
        for (int extent : sizes) occupied = addBounded(occupied,extent);
        const int extra = std::max(0,main_extent - occupied);
        int cursor = horizontal ? origin.x + padding : origin.y + padding;
        if (style_.justifyContent == JustifyContent::Center) cursor += extra / 2;
        if (style_.justifyContent == JustifyContent::End) cursor += extra;
        const int between = style_.justifyContent == JustifyContent::SpaceBetween && gaps > 0 ? extra / gaps : 0;
        int remainder = style_.justifyContent == JustifyContent::SpaceBetween && gaps > 0 ? extra % gaps : 0;
        int placed_children = 0;
        for (std::size_t i = 0; i < children_.size(); ++i)
        {
            if (children_[i]->type_ == Component::Type::Dialog)
            {
                children_[i]->layout(Constraints{.maximum = {}},{.x = origin.x,.y = origin.y});
                continue;
            }
            if (!children_[i]->tabPanelActive())
            {
                children_[i]->layout(Constraints{.maximum = {}},{.x = origin.x,.y = origin.y});
                continue;
            }
            const int cross_natural = horizontal ?
                children_[i]->measure(Constraints{.maximum = {.width = sizes[i],.height = cross_extent}}).height :
                children_[i]->measure(Constraints{.maximum = {.width = cross_extent,.height = sizes[i]}}).width;
            const int cross_size = style_.alignItems == AlignItems::Stretch ? cross_extent : cross_natural;
            int cross_position = horizontal ? origin.y + padding : origin.x + padding;
            if (style_.alignItems == AlignItems::Center) cross_position += (cross_extent - cross_size) / 2;
            if (style_.alignItems == AlignItems::End) cross_position += cross_extent - cross_size;
            const Size slot = horizontal ? Size{.width = sizes[i],.height = cross_size} :
                Size{.width = cross_size,.height = sizes[i]};
            children_[i]->layout(Constraints{.minimum = slot,.maximum = slot},
                horizontal ? Point{.x = cursor,.y = cross_position} : Point{.x = cross_position,.y = cursor});
            cursor += sizes[i];
            ++placed_children;
            if (placed_children < visible_children)
            {
                cursor += gap;
                cursor += between;
                if (remainder > 0) { ++cursor; --remainder; }
            }
        }
        return;
    }
    int x = origin.x + padding;
    int y = origin.y + padding;
    int placed_children = 0;
    for (const auto& child : children_)
    {
        if (child->type_ == Component::Type::Dialog || child->type_ == Component::Type::Toast)
        {
            child->layout(Constraints{.maximum = {}},{.x = origin.x,.y = origin.y});
            continue;
        }
        if (!child->tabPanelActive())
        {
            child->layout(Constraints{.maximum = {}},{.x = origin.x,.y = origin.y});
            continue;
        }
        const bool horizontal = type_ == Component::Type::Row;
        if (placed_children > 0)
        {
            if (horizontal) x += gap;
            else y += gap;
        }
        const Size available {.width = horizontal ? std::max(0,inner_width - (x - origin.x - padding)) : inner_width,
            .height = horizontal ? inner_height : std::max(0,inner_height - (y - origin.y - padding))};
        child->layout(Constraints{.maximum = available},{.x = x,.y = y});
        if (horizontal) x += child->bounds_.width;
        else y += child->bounds_.height;
        ++placed_children;
    }
}

bool RenderNode::handleChoiceKey(const Event& event)
{
    if (!interactiveControl()) return false;
    const bool activate = event.key == Key::Enter ||
        (event.key == Key::Character && event.character == U' ');
    if (type_ != Component::Type::RadioGroup)
    {
        if (!activate) return false;
        if (on_toggle_ && (type_ != Component::Type::Radio || !checked_)) on_toggle_(!checked_);
        return true;
    }
    if (radio_options_.empty()) return false;
    const int count = static_cast<int>(radio_options_.size());
    int selected = -1;
    for (int i = 0; i < count; ++i)
        if (radio_options_[i].value == text_) { selected = i; break; }
    int next = selected;
    if (event.key == Key::Up || event.key == Key::Left || event.key == Key::Down || event.key == Key::Right)
    {
        const int direction = event.key == Key::Up || event.key == Key::Left ? -1 : 1;
        for (int step = 1; step <= count; ++step)
        {
            const int candidate = ((selected < 0 ? (direction > 0 ? -1 : 0) : selected) +
                direction*step + count*(step+1)) % count;
            if (!radio_options_[candidate].disabled) { next = candidate; break; }
        }
    }
    else if (activate)
    {
        if (selected < 0)
            for (int i = 0; i < count; ++i)
                if (!radio_options_[i].disabled) { next = i; break; }
    }
    else return false;
    if (next >= 0 && !radio_options_[next].disabled && next != selected && on_change_)
        on_change_(radio_options_[next].value);
    return true;
}

bool RenderNode::handleChoiceClick(const Point position)
{
    if (!interactiveControl()) return false;
    if (type_ == Component::Type::RadioGroup)
    {
        const int index = position.y-bounds_.y;
        if (index < 0 || index >= static_cast<int>(radio_options_.size()) ||
            radio_options_[index].disabled) return true;
        if (radio_options_[index].value != text_ && on_change_)
            on_change_(radio_options_[index].value);
    }
    else if (on_toggle_ && (type_ != Component::Type::Radio || !checked_))
        on_toggle_(!checked_);
    return true;
}

bool RenderNode::handleSliderKey(const Event& event)
{
    if (!sliderInteractive()) return false;
    if (!std::isfinite(slider_min_) || !std::isfinite(slider_max_) || slider_max_ <= slider_min_) return false;
    const double step = std::isfinite(slider_step_) && slider_step_ > 0.0 ? slider_step_ : 1.0;
    const double current = std::isfinite(value_) ? std::clamp(value_,slider_min_,slider_max_) : slider_min_;
    double next = current;
    if (event.key == Key::Left || event.key == Key::Down) next = current-step;
    else if (event.key == Key::Right || event.key == Key::Up) next = current+step;
    else if (event.key == Key::PageDown) next = current-10*step;
    else if (event.key == Key::PageUp) next = current+10*step;
    else if (event.key == Key::Home) next = slider_min_;
    else if (event.key == Key::End) next = slider_max_;
    else return false;
    next = std::clamp(next,slider_min_,slider_max_);
    if (next != current) slider_on_change_(next);
    return true;
}

bool RenderNode::handleSliderMouse(const Point position)
{
    if (!sliderInteractive()) return false;
    if (!std::isfinite(slider_min_) || !std::isfinite(slider_max_) || slider_max_ <= slider_min_) return false;
    const int width = bounds_.width-(slider_show_value_ && bounds_.width >= 12 ? 8 : 0);
    const double fraction = width <= 1 ? 0.0 :
        static_cast<double>(std::clamp(position.x-bounds_.x,0,width-1))/(width-1);
    const double raw = slider_min_+fraction*(slider_max_-slider_min_);
    const double step = std::isfinite(slider_step_) && slider_step_ > 0.0 ? slider_step_ : 1.0;
    const double next = std::clamp(slider_min_+std::round((raw-slider_min_)/step)*step,
        slider_min_,slider_max_);
    const double current = std::isfinite(value_) ? std::clamp(value_,slider_min_,slider_max_) : slider_min_;
    if (next != current) slider_on_change_(next);
    return true;
}

bool RenderNode::handleTabsKey(const Event& event)
{
    if (type_ != Component::Type::Tabs || disabled_ || tab_options_.empty()) return false;
    const int count = static_cast<int>(tab_options_.size());
    int selected = -1;
    for (int i = 0; i < count; ++i)
        if (tab_options_[i].value == text_) { selected = i; break; }
    int next = selected;
    if (event.key == Key::Left || event.key == Key::Right)
    {
        const int direction = event.key == Key::Right ? 1 : -1;
        for (int step = 1; step <= count; ++step)
        {
            const int candidate = ((selected < 0 ? (direction > 0 ? -1 : 0) : selected) +
                direction*step + count*(step+1)) % count;
            if (!tab_options_[candidate].disabled) { next = candidate; break; }
        }
    }
    else if (event.key == Key::Home || event.key == Key::End)
    {
        for (int i = 0; i < count; ++i)
        {
            const int candidate = event.key == Key::Home ? i : count-1-i;
            if (!tab_options_[candidate].disabled) { next = candidate; break; }
        }
    }
    else if (event.key == Key::Enter || (event.key == Key::Character && event.character == U' '))
    {
        if (selected < 0)
            for (int i = 0; i < count; ++i)
                if (!tab_options_[i].disabled) { next = i; break; }
    }
    else return false;
    if (next >= 0 && !tab_options_[next].disabled && next != selected && on_change_)
        on_change_(tab_options_[next].value);
    return true;
}

bool RenderNode::handleTabsClick(const Point position)
{
    if (type_ != Component::Type::Tabs || disabled_ || position.y < bounds_.y ||
        position.y >= bounds_.y+bounds_.height) return false;
    int x = bounds_.x;
    for (const auto& option : tab_options_)
    {
        const int width = textWidth(option.label)+4;
        if (position.x >= x && position.x < x+width)
        {
            if (!option.disabled && option.value != text_ && on_change_) on_change_(option.value);
            return true;
        }
        x += width;
    }
    return false;
}

bool RenderNode::updateTabsHover(const Point position)
{
    if (type_ != Component::Type::Tabs) return false;
    int index = -1;
    if (!disabled_ && position.y >= bounds_.y && position.y < bounds_.y+bounds_.height)
    {
        int x = bounds_.x;
        for (std::size_t i = 0; i < tab_options_.size(); ++i)
        {
            const int width = textWidth(tab_options_[i].label)+4;
            if (position.x >= x && position.x < x+width)
            {
                if (!tab_options_[i].disabled) index = static_cast<int>(i);
                break;
            }
            x += width;
        }
    }
    if (index == hovered_tab_) return false;
    hovered_tab_ = index;
    return true;
}

std::vector<RenderNode::TreeVisibleItem> RenderNode::treeVisibleItems() const
{
    std::vector<TreeVisibleItem> result;
    const auto visit = [&](const auto& self, const std::vector<TreeItem>& items,
        const std::string& parent, const int depth, const std::vector<bool>& guides) -> void {
        for (std::size_t i = 0; i < items.size(); ++i)
        {
            const auto& item = items[i];
            const bool expanded = std::find(tree_expanded_.begin(),tree_expanded_.end(),item.key) !=
                tree_expanded_.end();
            const bool has_next = i+1 < items.size();
            result.push_back({.item = &item,.parent = parent,.depth = depth,
                .expanded = expanded,.has_next = has_next,.ancestor_has_next = guides});
            if (expanded)
            {
                auto child_guides = guides;
                child_guides.push_back(has_next);
                self(self,item.children,item.key,depth+1,child_guides);
            }
        }
    };
    visit(visit,tree_items_,{},0,{});
    return result;
}

Rect RenderNode::splitHandle() const noexcept
{
    if (type_ != Component::Type::ResizableSplit || children_.size() != 2) return {};
    const auto first = children_[0]->bounds_;
    return split_direction_ == SplitDirection::Horizontal ?
        Rect{.x = first.x+first.width,.y = bounds_.y,.width = 1,.height = bounds_.height} :
        Rect{.x = bounds_.x,.y = first.y+first.height,.width = bounds_.width,.height = 1};
}

RenderNode* RenderNode::splitTarget(const Point position) noexcept
{
    if (position.x < bounds_.x || position.y < bounds_.y ||
        position.x >= bounds_.x+bounds_.width || position.y >= bounds_.y+bounds_.height) return nullptr;
    if (type_ == Component::Type::ResizableSplit)
    {
        const auto handle = splitHandle();
        if (position.x >= handle.x && position.x < handle.x+handle.width &&
            position.y >= handle.y && position.y < handle.y+handle.height) return this;
    }
    for (auto it = children_.rbegin(); it != children_.rend(); ++it)
        if (auto* target = (*it)->splitTarget(position)) return target;
    return nullptr;
}

bool RenderNode::dragSplit(const Point position)
{
    if (type_ != Component::Type::ResizableSplit || !split_on_change_) return false;
    const bool horizontal = split_direction_ == SplitDirection::Horizontal;
    const int extent = horizontal ? bounds_.width : bounds_.height;
    const int available = std::max(0,extent-1);
    const int lower = std::min(split_min_first_,available);
    const int upper = std::max(lower,available-split_min_second_);
    const int proposed = (horizontal ? position.x-bounds_.x : position.y-bounds_.y);
    const int next = std::clamp(proposed,lower,upper);
    if (next != std::clamp(split_position_,lower,upper)) split_on_change_(next);
    return true;
}

bool RenderNode::handleSplitKey(const Event& event)
{
    if (type_ != Component::Type::ResizableSplit || !split_on_change_) return false;
    const bool horizontal = split_direction_ == SplitDirection::Horizontal;
    const int step = event.key == Key::PageUp || event.key == Key::PageDown ? 5 : 1;
    int delta = 0;
    if (event.key == (horizontal ? Key::Left : Key::Up) || event.key == Key::PageUp) delta = -step;
    else if (event.key == (horizontal ? Key::Right : Key::Down) || event.key == Key::PageDown) delta = step;
    else if (event.key == Key::Home) delta = -std::numeric_limits<int>::max()/2;
    else if (event.key == Key::End) delta = std::numeric_limits<int>::max()/2;
    else return false;
    return dragSplit(horizontal ? Point{.x = bounds_.x+split_position_+delta,.y = bounds_.y} :
        Point{.x = bounds_.x,.y = bounds_.y+split_position_+delta});
}

bool RenderNode::handleTreeKey(const Event& event)
{
    if (type_ != Component::Type::TreeView || disabled_) return false;
    const auto visible = treeVisibleItems();
    if (visible.empty()) return false;
    const int count = static_cast<int>(visible.size());
    int selected = -1;
    for (int i = 0; i < count; ++i)
        if (visible[i].item->key == text_) { selected = i; break; }
    int next = selected;
    int direction = 1;
    if (event.key == Key::Right || event.key == Key::Enter ||
        (event.key == Key::Character && event.character == U' '))
    {
        if (selected < 0) next = 0;
        else if (!visible[selected].item->children.empty() && !visible[selected].item->disabled)
        {
            if (!visible[selected].expanded && tree_on_toggle_)
                tree_on_toggle_(visible[selected].item->key,true);
            else if (event.key == Key::Right && selected+1 < count) next = selected+1;
        }
        else if (event.key == Key::Right) return true;
    }
    else if (event.key == Key::Left)
    {
        direction = -1;
        if (selected >= 0 && visible[selected].expanded &&
            !visible[selected].item->children.empty() && tree_on_toggle_)
            tree_on_toggle_(visible[selected].item->key,false);
        else if (selected >= 0 && !visible[selected].parent.empty())
            for (int i = 0; i < count; ++i)
                if (visible[i].item->key == visible[selected].parent) { next = i; break; }
    }
    else if (event.key == Key::Up || event.key == Key::Down ||
        event.key == Key::PageUp || event.key == Key::PageDown)
    {
        const int step = event.key == Key::PageUp || event.key == Key::PageDown ?
            std::max(1,bounds_.height-1) : 1;
        direction = event.key == Key::Up || event.key == Key::PageUp ? -1 : 1;
        next = selected < 0 ? (direction < 0 ? count-1 : 0) :
            std::clamp(selected+direction*step,0,count-1);
    }
    else if (event.key == Key::Home) { next = 0; direction = 1; }
    else if (event.key == Key::End) { next = count-1; direction = -1; }
    else return false;
    while (next >= 0 && next < count && visible[next].item->disabled) next += direction;
    if (next >= 0 && next < count)
    {
        const int height = std::max(1,bounds_.height);
        tree_scroll_.configure(count,height);
        tree_scroll_.revealItem(next);
        if (next != selected && on_change_) on_change_(visible[next].item->key);
    }
    return true;
}

bool RenderNode::handleTreeClick(const Point position)
{
    if (type_ != Component::Type::TreeView || disabled_ || position.x < bounds_.x ||
        position.x >= bounds_.x+bounds_.width || position.y < bounds_.y ||
        position.y >= bounds_.y+bounds_.height) return false;
    const auto visible = treeVisibleItems();
    const int scroll = tree_scroll_.offset();
    const int index = scroll+position.y-bounds_.y;
    if (index < 0 || index >= static_cast<int>(visible.size())) return false;
    const auto& entry = visible[index];
    if (entry.item->disabled) return true;
    if (!entry.item->children.empty() && position.x <= bounds_.x+entry.depth*2+2 && tree_on_toggle_)
    {
        if (entry.expanded && on_change_)
        {
            for (int i = index+1; i < static_cast<int>(visible.size()) &&
                visible[i].depth > entry.depth; ++i)
                if (visible[i].item->key == text_)
                {
                    on_change_(entry.item->key);
                    break;
                }
        }
        tree_on_toggle_(entry.item->key,!entry.expanded);
    }
    else if (entry.item->key != text_ && on_change_) on_change_(entry.item->key);
    return true;
}

std::vector<std::size_t> RenderNode::tableOrder() const
{
    std::vector<std::size_t> order;
    order.reserve(table_rows_.size());
    for (std::size_t i = 0; i < table_rows_.size(); ++i)
        if (!table_filter_ || table_filter_(table_rows_[i])) order.push_back(i);
    if (table_sort_column_ >= 0)
    {
        const auto column = static_cast<std::size_t>(table_sort_column_);
        std::stable_sort(order.begin(),order.end(),[&](const std::size_t a, const std::size_t b) {
            const auto& left = table_rows_[a].cells;
            const auto& right = table_rows_[b].cells;
            const std::string_view lhs = column < left.size() ? std::string_view(left[column]) : std::string_view{};
            const std::string_view rhs = column < right.size() ? std::string_view(right[column]) : std::string_view{};
            return table_sort_ascending_ ? lhs < rhs : rhs < lhs;
        });
    }
    return order;
}

bool RenderNode::handleTableKey(const Event& event)
{
    if (type_ != Component::Type::Table || disabled_) return false;
    const auto order = tableOrder();
    if (order.empty()) return false;
    int selected = -1;
    for (std::size_t i = 0; i < order.size(); ++i)
        if (table_rows_[order[i]].key == text_) { selected = static_cast<int>(i); break; }
    int next = selected;
    const int count = static_cast<int>(order.size());
    if (event.key == Key::Up || event.key == Key::Down || event.key == Key::PageUp ||
        event.key == Key::PageDown)
    {
        const int step = event.key == Key::PageUp || event.key == Key::PageDown ?
            std::max(1,bounds_.height-2) : 1;
        next = std::clamp(selected+(event.key == Key::Up || event.key == Key::PageUp ? -step : step),0,count-1);
        if (selected < 0) next = event.key == Key::Up || event.key == Key::PageUp ? count-1 : 0;
    }
    else if (event.key == Key::Home) next = 0;
    else if (event.key == Key::End) next = count-1;
    else return false;
    const int direction = next >= selected ? 1 : -1;
    while (next >= 0 && next < count && table_rows_[order[next]].disabled) next += direction;
    if (next < 0 || next >= count) return true;
    const int visible = std::max(1,bounds_.height-2);
    table_scroll_.configure(count,visible);
    table_scroll_.revealItem(next);
    if (next != selected && on_change_) on_change_(table_rows_[order[next]].key);
    return true;
}

bool RenderNode::handleTableClick(const Point position)
{
    if (type_ != Component::Type::Table || disabled_ || position.x < bounds_.x ||
        position.x >= bounds_.x+bounds_.width || position.y < bounds_.y ||
        position.y >= bounds_.y+bounds_.height) return false;
    if (position.y == bounds_.y)
    {
        int x = bounds_.x+1;
        for (std::size_t i = 0; i < table_columns_.size(); ++i)
        {
            int width = textWidth(table_columns_[i].title);
            for (const auto& row : table_rows_)
                if (i < row.cells.size()) width = std::max(width,textWidth(row.cells[i]));
            width = std::max(1,table_columns_[i].width > 0 ? table_columns_[i].width : width+2);
            if (position.x >= x && position.x < x+width)
            {
                if (table_sort_column_ == static_cast<int>(i)) table_sort_ascending_ = !table_sort_ascending_;
                else { table_sort_column_ = static_cast<int>(i); table_sort_ascending_ = true; }
                table_scroll_.setOffset(0);
                return true;
            }
            x += width+1;
        }
        return false;
    }
    if (position.y < bounds_.y+2 || position.x == scrollbarTrack().x) return false;
    const auto order = tableOrder();
    const int scroll = table_scroll_.offset();
    const int index = scroll+position.y-bounds_.y-2;
    if (index < 0 || index >= static_cast<int>(order.size())) return false;
    const auto& row = table_rows_[order[index]];
    if (!row.disabled && on_change_ && row.key != text_) on_change_(row.key);
    return true;
}

const RenderNode* RenderNode::hitTest(const Point position) const noexcept
{
    if (position.x < bounds_.x || position.y < bounds_.y ||
        position.x >= bounds_.x + bounds_.width || position.y >= bounds_.y + bounds_.height) return nullptr;
    for (auto it = children_.rbegin(); it != children_.rend(); ++it)
        if (const auto* hit = (*it)->hitTest(position)) return hit;
    return this;
}

RenderNode* RenderNode::scrollTarget(const Point position) noexcept
{
    if (position.x < bounds_.x || position.y < bounds_.y ||
        position.x >= bounds_.x + bounds_.width || position.y >= bounds_.y + bounds_.height) return nullptr;
    for (auto it = children_.rbegin(); it != children_.rend(); ++it)
        if (auto* target = (*it)->scrollTarget(position)) return target;
    return (type_ == Component::Type::ScrollView && on_scroll_) ||
        ((type_ == Component::Type::Table || type_ == Component::Type::TreeView) && !disabled_) ?
        this : nullptr;
}

bool RenderNode::scrollBy(const int delta)
{
    if (type_ == Component::Type::TreeView && !disabled_)
    {
        tree_scroll_.advance(delta);
        return true;
    }
    if (type_ == Component::Type::Table && !disabled_)
    {
        table_scroll_.advance(delta);
        return true;
    }
    if (type_ != Component::Type::ScrollView || !on_scroll_) return false;
    const auto scroll = scrollController();
    const int target = scroll.scrollBy(delta);
    if (target == scroll.offset()) return true;
    on_scroll_(target);
    return true;
}

bool RenderNode::handleScrollKey(const Event& event)
{
    if (type_ != Component::Type::ScrollView || !on_scroll_) return false;
    switch (event.key)
    {
        case Key::Up: return scrollBy(-1);
        case Key::Down: return scrollBy(1);
        case Key::PageUp: return scrollBy(-std::max(1,scroll_.viewport()-1));
        case Key::PageDown: return scrollBy(std::max(1,scroll_.viewport()-1));
        case Key::Home: return scrollBy(-std::numeric_limits<int>::max());
        case Key::End: return scrollBy(std::numeric_limits<int>::max());
        default: return false;
    }
}

RenderNode* RenderNode::contextTarget(const Point position) noexcept
{
    if (position.x < bounds_.x || position.y < bounds_.y ||
        position.x >= bounds_.x+bounds_.width || position.y >= bounds_.y+bounds_.height ||
        !tabPanelActive()) return nullptr;
    for (auto it = children_.rbegin(); it != children_.rend(); ++it)
        if (auto* child = (*it)->contextTarget(position)) return child;
    return type_ == Component::Type::ContextMenu && !disabled_ && !select_options_.empty() ? this : nullptr;
}

bool RenderNode::openContextMenu(const Point position, const Size screen)
{
    if (type_ != Component::Type::ContextMenu || disabled_ || select_options_.empty()) return false;
    context_anchor_ = position;
    select_open_ = true;
    select_drag_.end();
    select_scroll_.setOffset(0);
    select_highlight_ = 0;
    for (std::size_t i = 0; i < select_options_.size(); ++i)
        if (!select_options_[i].disabled) { select_highlight_ = static_cast<int>(i); break; }
    const int viewport = std::max(1,selectPopupRect(screen).height);
    select_scroll_.configure(static_cast<int>(select_options_.size()),viewport);
    select_scroll_.setOffset(select_highlight_-viewport/2);
    return true;
}

Rect RenderNode::selectPopupRect(const Size screen) const noexcept
{
    if (!selectOpen() || select_options_.empty()) return {};
    if (type_ == Component::Type::ContextMenu)
    {
        int width = 8;
        for (const auto& option : select_options_) width = std::max(width,textWidth(option.label)+3);
        width = std::min(width,std::max(0,screen.width));
        const int height = std::min({10,static_cast<int>(select_options_.size()),std::max(0,screen.height)});
        if (height == 0 || width == 0) return {};
        return {.x = std::clamp(context_anchor_.x,0,screen.width-width),
            .y = std::clamp(context_anchor_.y,0,screen.height-height),.width = width,.height = height};
    }
    const int below = std::max(0,screen.height - std::max(0,bounds_.y+bounds_.height));
    const int above = std::clamp(bounds_.y,0,screen.height);
    const int height = std::min({6,static_cast<int>(select_options_.size()),std::max(below,above)});
    int width = bounds_.width;
    if (type_ == Component::Type::DropdownMenu)
        for (const auto& option : select_options_) width = std::max(width,textWidth(option.label)+3);
    width = std::min(width,std::max(0,screen.width));
    if (height <= 0 || width <= 0) return {};
    const int y = below >= height ? bounds_.y+bounds_.height : bounds_.y-height;
    return {.x = std::clamp(bounds_.x,0,screen.width-width),.y = y,.width = width,.height = height};
}

Rect RenderNode::selectScrollbarTrack(const Size screen) const noexcept
{
    const Rect popup = selectPopupRect(screen);
    if (popup.width < 3 || popup.height <= 0 || static_cast<int>(select_options_.size()) <= popup.height) return {};
    return {.x = popup.x+popup.width-1,.y = popup.y,.width = 1,.height = popup.height};
}

Rect RenderNode::selectScrollbarThumb(const Size screen) const noexcept
{
    const Rect track = selectScrollbarTrack(screen);
    return select_scroll_.thumb(track);
}

bool RenderNode::selectContains(const Point point, const Size screen) const noexcept
{
    const Rect popup = selectPopupRect(screen);
    return popup.width > 0 && point.x >= popup.x && point.x < popup.x + popup.width &&
        point.y >= popup.y && point.y < popup.y + popup.height;
}

bool RenderNode::handleSelectKey(const Event& event, const Size screen)
{
    if ((type_ != Component::Type::Select && type_ != Component::Type::DropdownMenu &&
        type_ != Component::Type::ContextMenu) || disabled_) return false;
    if (!select_open_)
    {
        if (type_ == Component::Type::ContextMenu) return false;
        if (event.key != Key::Enter && event.key != Key::Down &&
            !(event.key == Key::Character && event.character == U' ')) return false;
        if (select_options_.empty()) return true;
        select_open_ = true;
        select_highlight_ = 0;
        for (std::size_t i = 0; i < select_options_.size(); ++i)
            if (type_ == Component::Type::Select && select_options_[i].value == text_)
            { select_highlight_ = static_cast<int>(i); break; }
        if (type_ == Component::Type::DropdownMenu)
            for (std::size_t i = 0; i < select_options_.size(); ++i)
                if (!select_options_[i].disabled) { select_highlight_ = static_cast<int>(i); break; }
        const int viewport = std::max(1,selectPopupRect(screen).height);
        select_scroll_.configure(static_cast<int>(select_options_.size()),viewport);
        select_scroll_.setOffset(select_highlight_-viewport/2);
    }
    else
    {
        if (event.key == Key::Escape) { closeSelect(); return true; }
        if (event.key == Key::Up || event.key == Key::Down || event.key == Key::Home ||
            event.key == Key::End || event.key == Key::PageUp || event.key == Key::PageDown)
        {
            const int count = static_cast<int>(select_options_.size());
            int next = select_highlight_;
            if (event.key == Key::Up) --next;
            if (event.key == Key::Down) ++next;
            if (event.key == Key::Home) next = 0;
            if (event.key == Key::End) next = count-1;
            if (event.key == Key::PageUp) next -= std::max(1,selectPopupRect(screen).height);
            if (event.key == Key::PageDown) next += std::max(1,selectPopupRect(screen).height);
            select_highlight_ = std::clamp(next,0,count-1);
            if (type_ != Component::Type::Select && select_options_[select_highlight_].disabled)
            {
                const int direction = event.key == Key::Up || event.key == Key::PageUp ||
                    event.key == Key::End ? -1 : 1;
                for (int i = select_highlight_; i >= 0 && i < count; i += direction)
                    if (!select_options_[i].disabled) { select_highlight_ = i; break; }
            }
        }
        else if (event.key == Key::Enter || (event.key == Key::Character && event.character == U' '))
        {
            if (!select_options_[select_highlight_].disabled && on_change_)
                on_change_(select_options_[select_highlight_].value);
            closeSelect();
            return true;
        }
        else return false;
    }
    const int viewport = std::max(1,selectPopupRect(screen).height);
    select_scroll_.configure(static_cast<int>(select_options_.size()),viewport);
    select_scroll_.revealItem(select_highlight_);
    return true;
}

bool RenderNode::handleSelectMouse(const MouseEvent& mouse, const Size screen)
{
    if ((type_ != Component::Type::Select && type_ != Component::Type::DropdownMenu &&
        type_ != Component::Type::ContextMenu) || disabled_) return false;
    if (select_drag_.active())
    {
        if (mouse.action == MouseAction::Release) { select_drag_.end(); return true; }
        if (mouse.action == MouseAction::Move)
        {
            const Rect track = selectScrollbarTrack(screen);
            select_scroll_.setOffset(select_drag_.move(select_scroll_,track,mouse.position.y));
            return true;
        }
    }
    if (selectContains(mouse.position,screen) && select_open_)
    {
        const Rect popup = selectPopupRect(screen);
        const Rect track = selectScrollbarTrack(screen);
        if (mouse.action == MouseAction::Press && mouse.button == MouseButton::Left &&
            track.width > 0 && mouse.position.x == track.x)
        {
            int target = select_scroll_.offset();
            if (select_drag_.begin(select_scroll_,track,mouse.position,target)) select_scroll_.setOffset(target);
            return true;
        }
        if (mouse.action == MouseAction::Scroll)
        {
            select_scroll_.advance(-mouse.scrollY);
            return true;
        }
        if (mouse.action == MouseAction::Move)
        {
            select_highlight_ = std::min(static_cast<int>(select_options_.size())-1,
                select_scroll_.offset()+mouse.position.y-popup.y);
            return true;
        }
        if (mouse.action == MouseAction::Press && mouse.button == MouseButton::Left)
        {
            const int index = select_scroll_.offset()+mouse.position.y-popup.y;
            if (index < static_cast<int>(select_options_.size()) && !select_options_[index].disabled)
            {
                if (on_change_) on_change_(select_options_[index].value);
                closeSelect();
            }
        }
        return true;
    }
    if (type_ == Component::Type::ContextMenu) return false;
    if (mouse.action == MouseAction::Press && mouse.button == MouseButton::Left)
    {
        if (mouse.position.x >= bounds_.x && mouse.position.x < bounds_.x+bounds_.width &&
            mouse.position.y >= bounds_.y && mouse.position.y < bounds_.y+bounds_.height)
        {
            if (select_open_) closeSelect();
            else
            {
                select_open_ = true;
                select_highlight_ = 0;
                for (std::size_t i = 0; i < select_options_.size(); ++i)
                    if (type_ == Component::Type::Select && select_options_[i].value == text_)
                    { select_highlight_ = static_cast<int>(i); break; }
                if (type_ == Component::Type::DropdownMenu)
                    for (std::size_t i = 0; i < select_options_.size(); ++i)
                        if (!select_options_[i].disabled) { select_highlight_ = static_cast<int>(i); break; }
                const int viewport = std::max(1,selectPopupRect(screen).height);
                select_scroll_.configure(static_cast<int>(select_options_.size()),viewport);
                select_scroll_.setOffset(select_highlight_-viewport/2);
            }
            return true;
        }
        closeSelect();
    }
    return false;
}

void RenderNode::paintOverlays(Painter& painter, const Size screen, const Theme& theme) const
{
    for (const auto& child : children_) child->paintOverlays(painter,screen,theme);
    paintOverlay(painter,screen,theme);
}

bool RenderNode::tooltipContains(const Point point) const noexcept
{
    return type_ == Component::Type::Tooltip && !disabled_ && !text_.empty() &&
        point.x >= bounds_.x && point.x < bounds_.x+bounds_.width &&
        point.y >= bounds_.y && point.y < bounds_.y+bounds_.height;
}

Rect RenderNode::tooltipRect(const Point point, const Size screen) const noexcept
{
    if (!tooltipContains(point) || screen.width <= 0 || screen.height <= 0) return {};
    const int height = screen.height >= 3 ? 3 : 1;
    const int width = std::min(screen.width,textWidth(text_)+(height == 3 ? 4 : 0));
    if (width <= 0) return {};
    int y = point.y+1;
    if (y+height > screen.height) y = point.y-height;
    return {.x = std::clamp(point.x+1,0,screen.width-width),
        .y = std::clamp(y,0,screen.height-height),.width = width,.height = height};
}

void RenderNode::paintTooltip(Painter& painter, const Point point, const Size screen,
    const Theme& theme) const
{
    const Rect popup = tooltipRect(point,screen);
    if (popup.width <= 0 || popup.height <= 0) return;
    const auto& palette = theme.palette;
    const TextStyle style {.foreground = palette.foreground,.background = palette.surface};
    painter.fillBackground(popup,palette.surface);
    if (popup.height >= 3)
    {
        TextStyle border_style = style;
        border_style.foreground = palette.muted;
        painter.border(popup,BorderStyle::Rounded,border_style);
        painter.text(popup.x+2,popup.y+1,clipText(text_,std::max(0,popup.width-4)),style);
    }
    else painter.text(popup.x,popup.y,clipText(text_,popup.width),style);
}

Rect RenderNode::toastRect(const Size screen, const int bottom_offset) const noexcept
{
    if (!toastOpen() || screen.width <= 0 || screen.height <= bottom_offset) return {};
    const int height = screen.height-bottom_offset >= 3 ? 3 : 1;
    const int width = std::min(std::max(1,screen.width-2),std::max(12,textWidth(text_)+7));
    return {.x = std::max(0,screen.width-width-1),
        .y = screen.height-bottom_offset-height,.width = width,.height = height};
}

void RenderNode::paintToast(Painter& painter, const Size screen, const Theme& theme,
    const int bottom_offset) const
{
    const Rect popup = toastRect(screen,bottom_offset);
    if (popup.width <= 0 || popup.height <= 0) return;
    const auto& palette = theme.palette;
    const Color accent = toast_severity_ == 1 ? Color::rgb(85,200,125) :
        toast_severity_ == 2 ? Color::rgb(240,185,80) :
        toast_severity_ == 3 ? Color::rgb(235,95,105) : palette.primary;
    const TextStyle style {.foreground = palette.foreground,.background = palette.surface};
    TextStyle emphasis = style;
    emphasis.foreground = accent;
    emphasis.bold = true;
    painter.fillBackground(popup,palette.surface);
    const int y = popup.y+(popup.height >= 3 ? 1 : 0);
    if (popup.height >= 3) painter.border(popup,BorderStyle::Rounded,emphasis);
    const int inset = popup.height >= 3 ? 2 : 0;
    const std::string_view indicator = toast_severity_ == 1 ? "✓" :
        toast_severity_ == 2 ? "!" : toast_severity_ == 3 ? "×" : "i";
    painter.text(popup.x+inset,y,indicator,emphasis);
    painter.text(popup.x+inset+2,y,clipText(text_,std::max(0,popup.width-inset*2-2)),style);
}

void RenderNode::closeToast()
{
    if (!toastOpen()) return;
    toast_visible_ = false;
    if (toast_on_close_) toast_on_close_();
}

bool RenderNode::expireToast(const std::chrono::steady_clock::time_point now)
{
    if (!toastOpen() || toast_duration_ms_ == 0 ||
        now-toast_started_ < std::chrono::milliseconds(toast_duration_ms_)) return false;
    closeToast();
    return true;
}

void RenderNode::paintOverlay(Painter& painter, const Size screen, const Theme& theme) const
{
    const Rect popup = selectPopupRect(screen);
    if (popup.width <= 0 || popup.height <= 0) return;
    const Palette palette = theme.palette;
    const Rect track = selectScrollbarTrack(screen);
    const Rect thumb = selectScrollbarThumb(screen);
    painter.fill(popup,Cell{.grapheme = " ",.style = {.background = palette.surface}});
    for (int i = 0; i < popup.height; ++i)
    {
        const int index = select_scroll_.offset()+i;
        if (index >= static_cast<int>(select_options_.size())) break;
        const bool highlighted = index == select_highlight_;
        const Color background = highlighted ? palette.primary : palette.surface;
        painter.fill({.x = popup.x,.y = popup.y+i,.width = popup.width,.height = 1},
            Cell{.grapheme = " ",.style = {.background = background}});
        const TextStyle style {.foreground = select_options_[index].disabled ? palette.muted :
            highlighted ? palette.on_primary : palette.foreground,.background = background};
        painter.text(popup.x+1,popup.y+i,clipText(select_options_[index].label,
            std::max(0,popup.width-2-(track.width > 0 ? 1 : 0))),style);
    }
    paintScrollbar(painter,track,thumb,theme,palette.surface,true,
        select_drag_.active() ? ScrollbarInteraction::Active : ScrollbarInteraction::Normal);
}

const RenderNode* RenderNode::findById(const std::uint64_t id) const noexcept
{
    if (id_ == id) return this;
    for (const auto& child : children_)
        if (const auto* found = child->findById(id)) return found;
    return nullptr;
}

RenderNode* RenderNode::findById(const std::uint64_t id) noexcept
{
    return const_cast<RenderNode*>(static_cast<const RenderNode&>(*this).findById(id));
}

bool RenderNode::propagateEvent(const Event& event, const std::uint64_t target) const
{
    std::vector<const RenderNode*> path;
    const auto visit = [&](const auto& self, const RenderNode& node) -> bool {
        path.push_back(&node);
        if (node.id_ == target) return true;
        for (const auto& child : node.children_)
            if (self(self,*child)) return true;
        path.pop_back();
        return false;
    };
    if (!visit(visit,*this)) return false;
    ComponentEvent propagated(event,target);
    for (std::size_t i = 0; i + 1 < path.size(); ++i)
    {
        propagated.currentTarget = path[i]->id_;
        propagated.phase = EventPhase::Capture;
        if (path[i]->on_event_capture_) path[i]->on_event_capture_(propagated);
        if (propagated.propagationStopped()) return propagated.defaultPrevented();
    }
    propagated.currentTarget = target;
    propagated.phase = EventPhase::Target;
    if (path.back()->on_event_capture_) path.back()->on_event_capture_(propagated);
    if (!propagated.propagationStopped() && path.back()->on_event_)
        path.back()->on_event_(propagated);
    if (propagated.propagationStopped()) return propagated.defaultPrevented();
    for (std::size_t i = path.size() - 1; i > 0; --i)
    {
        propagated.currentTarget = path[i-1]->id_;
        propagated.phase = EventPhase::Bubble;
        if (path[i-1]->on_event_) path[i-1]->on_event_(propagated);
        if (propagated.propagationStopped()) break;
    }
    return propagated.defaultPrevented();
}

Rect RenderNode::dialogRect(const Size screen) const
{
    if (!dialogOpen() || screen.width <= 0 || screen.height <= 0) return {};
    const int width = std::min(screen.width,std::max(20,dialog_width_));
    int contents_height = 0;
    for (const auto& child : children_)
        contents_height += child->measure(Constraints{.maximum = {.width = std::max(0,width-4),
            .height = 10000}}).height + 1;
    if (!children_.empty()) --contents_height;
    const int header_height = text_.empty() ? 3 : 5;
    const int footer_height = dialog_show_actions_ ? 3 : 1;
    const int minimum_height = dialog_show_actions_ ? 9 : 5;
    const int height = std::min(screen.height,std::max(minimum_height,
        header_height+contents_height+footer_height));
    return {.x = (screen.width-width)/2,.y = (screen.height-height)/2,
        .width = width,.height = height};
}

void RenderNode::layoutDialogChildren(const Size screen)
{
    if (!dialogOpen()) return;
    dialog_screen_ = screen;
    const Rect rect = dialogRect(screen);
    const int top = rect.y + (text_.empty() ? 3 : 5);
    const int available_width = std::max(0,rect.width-4);
    const int content_bottom = rect.y+rect.height-(dialog_show_actions_ ? 3 : 1);
    dialog_content_rect_ = {.x = rect.x+2,.y = top,.width = available_width,
        .height = std::max(0,content_bottom-top)};
    int total_height = 0;
    for (const auto& child : children_)
        total_height += child->measure(Constraints{.maximum = {.width = available_width,
            .height = 10000}}).height+1;
    if (!children_.empty()) --total_height;
    dialog_scroll_.configure(total_height,dialog_content_rect_.height);
    int y = top-dialog_scroll_.offset();
    for (auto& child : children_)
    {
        child->layout(Constraints{.maximum = {.width = available_width,.height = 10000}},
            {.x = rect.x+2,.y = y});
        y += child->bounds().height + 1;
    }
}

bool RenderNode::scrollDialog(const int delta, const Size screen)
{
    if (!dialogOpen()) return false;
    const int next = scrollController().scrollBy(delta);
    if (next == dialog_scroll_.offset()) return false;
    dialog_scroll_.setOffset(next);
    layoutDialogChildren(screen);
    return true;
}

bool RenderNode::revealDialogFocus(const std::uint64_t id, const Size screen)
{
    if (!dialogOpen() || dialog_content_rect_.height <= 0) return false;
    const auto find = [&](const auto& self, const RenderNode& node) -> const RenderNode* {
        if (node.id() == id) return &node;
        for (const auto& child : node.children())
            if (const auto* found = self(self,*child)) return found;
        return nullptr;
    };
    if (const auto* target = find(find,*this))
    {
        if (target->bounds().y < dialog_content_rect_.y)
            return scrollDialog(target->bounds().y-dialog_content_rect_.y,screen);
        const int bottom = dialog_content_rect_.y+dialog_content_rect_.height;
        if (target->bounds().y+target->bounds().height > bottom)
            return scrollDialog(target->bounds().y+target->bounds().height-bottom,screen);
    }
    return false;
}

RenderNode* RenderNode::dialogChildAt(const Point point) noexcept
{
    if (point.x < dialog_content_rect_.x || point.x >= dialog_content_rect_.x+dialog_content_rect_.width ||
        point.y < dialog_content_rect_.y || point.y >= dialog_content_rect_.y+dialog_content_rect_.height)
        return nullptr;
    for (auto it = children_.rbegin(); it != children_.rend(); ++it)
        if (auto* hit = (*it)->hitTest(point)) return hit;
    return nullptr;
}

bool RenderNode::handleDialogKey(const Event& event)
{
    if (!dialogOpen()) return false;
    if (event.key == Key::Escape)
    {
        if (dialog_on_close_) dialog_on_close_();
    }
    else if (event.key == Key::Up || event.key == Key::Down || event.key == Key::PageUp ||
        event.key == Key::PageDown || event.key == Key::Home || event.key == Key::End)
    {
        const int page = std::max(1,dialog_content_rect_.height-1);
        const int delta = event.key == Key::Up ? -1 : event.key == Key::Down ? 1 :
            event.key == Key::PageUp ? -page : event.key == Key::PageDown ? page :
            event.key == Key::Home ? -dialog_scroll_.maximum() : dialog_scroll_.maximum();
        (void)scrollDialog(delta,dialog_screen_);
    }
    else if (dialog_show_actions_ && (event.key == Key::Tab || event.key == Key::Left || event.key == Key::Right))
        dialog_confirm_focused_ = !dialog_confirm_focused_;
    else if (dialog_show_actions_ && (event.key == Key::Enter || (event.key == Key::Character && event.character == U' ')))
    {
        const auto& action = dialog_confirm_focused_ ? on_click_ : dialog_on_close_;
        if (action) action();
    }
    return true;
}

bool RenderNode::handleDialogMouse(const MouseEvent& mouse, const Size screen)
{
    if (!dialogOpen()) return false;
    if (scrollbar_drag_.active())
    {
        if (mouse.action == MouseAction::Release) { scrollbar_drag_.end(); return true; }
        if (mouse.action == MouseAction::Move)
            return moveScrollbarDrag(mouse.position.y);
    }
    const Rect rect = dialogRect(screen);
    const bool inside = mouse.position.x >= rect.x && mouse.position.x < rect.x+rect.width &&
        mouse.position.y >= rect.y && mouse.position.y < rect.y+rect.height;
    if (mouse.action == MouseAction::Scroll)
    {
        if (inside) (void)scrollDialog(-mouse.scrollY*3,screen);
        return true;
    }
    if (mouse.action == MouseAction::Press && mouse.button == MouseButton::Left &&
        beginScrollbarDrag(mouse.position))
    {
        dialog_pressed_child_ = 0;
        return true;
    }
    if (auto* child = dialogChildAt(mouse.position))
    {
        if (mouse.action == MouseAction::Press && mouse.button == MouseButton::Left)
        {
            dialog_pressed_child_ = child->id();
            if (child->type() == Component::Type::TextField)
                child->setCursorFromX(mouse.position.x);
            return true;
        }
        if (mouse.action == MouseAction::Release)
        {
            if (dialog_pressed_child_ == child->id() && child->type() == Component::Type::Button &&
                !child->disabled() && child->onClick()) child->onClick()();
            if (dialog_pressed_child_ == child->id() && child->interactiveControl())
                child->handleChoiceClick(mouse.position);
            dialog_pressed_child_ = 0;
            return true;
        }
    }
    if (mouse.action == MouseAction::Release) { dialog_pressed_child_ = 0; return true; }
    if (mouse.action != MouseAction::Press || mouse.button != MouseButton::Left) return true;
    dialog_pressed_child_ = 0;
    if (!inside)
    {
        if (dialog_close_on_backdrop_ && dialog_on_close_) dialog_on_close_();
        return true;
    }
    if (dialog_show_actions_ && rect.height >= 5 && mouse.position.y == rect.y+rect.height-2)
    {
        const int confirm_x = rect.x+std::max(1,rect.width-static_cast<int>(dialog_confirm_text_.size())-3);
        const int cancel_x = rect.x+2;
        if (mouse.position.x >= confirm_x &&
            mouse.position.x < confirm_x+static_cast<int>(dialog_confirm_text_.size()))
        {
            dialog_confirm_focused_ = true;
            if (on_click_) on_click_();
        }
        else if (mouse.position.x >= cancel_x &&
            mouse.position.x < cancel_x+static_cast<int>(dialog_cancel_text_.size()))
        {
            dialog_confirm_focused_ = false;
            if (dialog_on_close_) dialog_on_close_();
        }
    }
    return true;
}

void RenderNode::paintDialog(Painter& painter, const Size screen, const Theme& theme,
    const std::string_view focused_id) const
{
    const Theme& local_theme = resolvedTheme(theme);
    const Rect rect = dialogRect(screen);
    if (rect.width <= 0 || rect.height <= 0) return;
    const Palette palette = local_theme.palette;
    painter.dim({.width = screen.width,.height = screen.height},palette.background,75);
    const TextStyle style {.foreground = palette.foreground,.background = palette.surface};
    painter.fill(rect,Cell{.grapheme = " ",.style = style});
    if (rect.width >= 2 && rect.height >= 2) painter.border(rect,BorderStyle::Rounded,style);
    if (rect.height >= 3)
    {
        TextStyle title_style = style;
        title_style.foreground = palette.primary;
        title_style.bold = true;
        painter.text(rect.x+2,rect.y+1,clipText(dialog_title_,std::max(0,rect.width-4)),title_style);
    }
    if (rect.height >= 5)
    {
        if (!text_.empty()) painter.text(rect.x+2,rect.y+3,clipText(text_,std::max(0,rect.width-4)),style);
        {
            const int top = rect.y+(text_.empty() ? 3 : 5);
            const int bottom = rect.y+rect.height-(dialog_show_actions_ ? 3 : 1);
            Painter::ClipScope clip(painter,{.x = rect.x+1,.y = top,
                .width = std::max(0,rect.width-2),.height = std::max(0,bottom-top)});
            for (const auto& child : children_) child->paint(painter,focused_id,{},palette.surface,&local_theme);
        }
        paintScrollbar(painter,scrollbarTrack(),scrollbarThumb(),local_theme,palette.surface,true,
            scrollbar_drag_.active() ? ScrollbarInteraction::Active : ScrollbarInteraction::Normal);
        if (!dialog_show_actions_) return;
        const int row = rect.y+rect.height-2;
        const int confirm_x = rect.x+std::max(1,rect.width-static_cast<int>(dialog_confirm_text_.size())-3);
        TextStyle cancel_style = style;
        TextStyle confirm_style = style;
        if (dialog_confirm_focused_)
        {
            confirm_style.background = palette.primary;
            confirm_style.foreground = palette.on_primary;
        }
        else
        {
            cancel_style.background = palette.primary;
            cancel_style.foreground = palette.on_primary;
        }
        painter.text(rect.x+2,row,clipText(dialog_cancel_text_,std::max(0,confirm_x-rect.x-4)),cancel_style);
        painter.text(confirm_x,row,clipText(dialog_confirm_text_,std::max(0,rect.x+rect.width-confirm_x-1)),confirm_style);
    }
}

void RenderNode::paint(Painter& painter, const std::string_view focused_id,
    const std::string_view hovered_id, const std::optional<Color> background, const Theme* theme,
    const bool unicode) const
{
    if (type_ == Component::Type::Dialog || type_ == Component::Type::Toast ||
        (type_ == Component::Type::TabPanel && !tab_panel_active_)) return;
    if (theme_override_) theme = &*theme_override_;
    std::optional<Painter::ClipScope> clipping;
    if (style_.clip || type_ == Component::Type::ScrollView) clipping.emplace(painter,bounds_);
    std::optional<Color> effective_background = background;
    if (surface_tone_ != SurfaceTone::None)
    {
        const Palette palette = theme ? theme->palette : Theme::dark().palette;
        effective_background = surface_tone_ == SurfaceTone::Primary ? palette.primary : palette.surface;
    }
    if (background_) effective_background = background_;
    TextStyle style = text_style_;
    if (theme && text_style_ == TextStyle{})
    {
        style.foreground = theme->palette.foreground;
        style.background = theme->palette.background;
    }
    if (theme && color_variant_ != ColorVariant::Default && text_style_ == TextStyle{})
        style.foreground = theme->palette.resolve(color_variant_);
    if (effective_background) style.background = *effective_background;

    if (effective_background && (background_ || surface_tone_ != SurfaceTone::None))
        painter.fillBackground(bounds_,*effective_background);
    if (type_ == Component::Type::Divider)
    {
        painter.horizontalLine(bounds_.x,bounds_.y,bounds_.width,style);
        return;
    }
    if (type_ == Component::Type::Progress)
    {
        if (bounds_.width <= 0 || bounds_.height <= 0) return;
        const double value = std::isfinite(value_) ? std::clamp(value_,0.0,1.0) : 0.0;
        const int suffix_width = progress_show_percentage_ && !progress_indeterminate_ &&
            bounds_.width >= 7 ? 5 : 0;
        const int bar_width = bounds_.width-suffix_width;
        const int filled = static_cast<int>(bar_width*value);
        const int segment_width = std::max(1,bar_width/4);
        const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
        const int phase = progress_indeterminate_ ?
            static_cast<int>((elapsed/90) % std::max(1,bar_width+segment_width)) - segment_width : 0;
        const bool ascii = !unicode || progress_variant_ == ProgressVariant::Ascii;
        const std::string_view on = ascii ? "#" :
            progress_variant_ == ProgressVariant::Line ? "━" : "█";
        const std::string_view off = ascii ? "-" :
            progress_variant_ == ProgressVariant::Line ? "─" : "░";
        for (int i = 0; i < bar_width; ++i)
        {
            const bool active = progress_indeterminate_ ? i >= phase && i < phase+segment_width : i < filled;
            TextStyle segment_style = style;
            if (theme) segment_style.foreground = active ?
                theme->palette.resolve(color_variant_) : theme->palette.muted;
            painter.put(bounds_.x+i,bounds_.y,active ? on : off,segment_style);
        }
        if (suffix_width > 0)
            painter.text(bounds_.x+bar_width,bounds_.y,
                " " + std::to_string(static_cast<int>(std::lround(value*100.0))) + "%",style);
        return;
    }
    if (type_ == Component::Type::Slider)
    {
        if (bounds_.width <= 0 || bounds_.height <= 0) return;
        const bool valid = std::isfinite(slider_min_) && std::isfinite(slider_max_) &&
            slider_max_ > slider_min_;
        const double value = std::isfinite(value_) ? value_ : slider_min_;
        const double fraction = valid ?
            std::clamp((value-slider_min_)/(slider_max_-slider_min_),0.0,1.0) : 0.0;
        const int label_width = slider_show_value_ && bounds_.width >= 12 ? 8 : 0;
        const int width = bounds_.width-label_width;
        const int thumb = width <= 1 ? 0 : static_cast<int>(std::lround(fraction*(width-1)));
        const Palette palette = theme ? theme->palette : Theme::dark().palette;
        for (int i = 0; i < width; ++i)
        {
            const bool active = i <= thumb;
            const Color foreground = disabled_ ? palette.muted : active ?
                palette.resolve(color_variant_) : palette.muted;
            const TextStyle segment{.foreground = foreground,.background = style.background};
            painter.put(bounds_.x+i,bounds_.y,i == thumb ? (unicode ? "●" : "O") :
                active ? (unicode ? "━" : "=") : (unicode ? "─" : "-"),segment);
        }
        if (label_width)
        {
            const std::string label = std::to_string(static_cast<int>(std::lround(value)));
            painter.text(bounds_.x+width+1,bounds_.y,label,style);
        }
        return;
    }
    if (type_ == Component::Type::TreeView)
    {
        if (bounds_.width <= 0 || bounds_.height <= 0) return;
        const Palette palette = theme ? theme->palette : Theme::dark().palette;
        const Color base = effective_background.value_or(palette.background);
        painter.fillBackground(bounds_,base);
        Painter::ClipScope clip(painter,bounds_);
        const auto items = treeVisibleItems();
        const int scroll = tree_scroll_.offset();
        const int available_width = bounds_.width-(scrollbarTrack().height > 0 ? 1 : 0);
        for (int i = 0; i < bounds_.height && scroll+i < static_cast<int>(items.size()); ++i)
        {
            const auto& entry = items[scroll+i];
            const int y = bounds_.y+i;
            const bool selected = entry.item->key == text_;
            const Color bg = selected ? blendColor(base,palette.primary,20) : base;
            painter.fillBackground({.x = bounds_.x,.y = y,.width = available_width,.height = 1},bg);
            const TextStyle style {.foreground = entry.item->disabled ? palette.muted :
                selected ? palette.primary : palette.foreground,.background = bg,.bold = selected};
            const int x = bounds_.x+entry.depth*2;
            const TextStyle guide_style {.foreground = palette.muted,.background = bg};
            for (int depth = 0; depth < entry.depth-1; ++depth)
                if (entry.ancestor_has_next[static_cast<std::size_t>(depth)])
                    painter.text(bounds_.x+depth*2+1,y,unicode ? "│" : "|",guide_style);
            if (entry.depth > 0)
                painter.text(x-1,y,entry.has_next ? (unicode ? "├" : "+") :
                    (unicode ? "└" : "\\"),guide_style);
            if (!entry.item->children.empty())
                painter.text(x,y,entry.expanded ? (unicode ? "▼" : "v") :
                    (unicode ? "▶" : ">"),style);
            else painter.text(x,y,unicode ? "·" : "-",style);
            const std::string prefix = !unicode || entry.item->icon.empty() ? "" : entry.item->icon+" ";
            painter.text(x+2,y,clipText(prefix+entry.item->label,
                std::max(0,bounds_.x+available_width-x-2)),style);
        }
        const Rect track = scrollbarTrack();
        if (track.height > 0)
        {
            paintScrollbar(painter,track,scrollbarThumb(),theme ? *theme : Theme::dark(),
                palette.surface,unicode,scrollbar_drag_.active() ? ScrollbarInteraction::Active :
                    hovered_id == std::to_string(id_) ? ScrollbarInteraction::Hover : ScrollbarInteraction::Normal);
        }
        return;
    }
    if (type_ == Component::Type::Table)
    {
        if (bounds_.width <= 0 || bounds_.height <= 0) return;
        const Palette palette = theme ? theme->palette : Theme::dark().palette;
        const Color base = effective_background.value_or(palette.background);
        painter.fillBackground(bounds_,base);
        Painter::ClipScope clip(painter,bounds_);
        std::vector<int> widths;
        widths.reserve(table_columns_.size());
        for (std::size_t i = 0; i < table_columns_.size(); ++i)
        {
            int width = textWidth(table_columns_[i].title);
            for (const auto& row : table_rows_)
            {
                if (table_filter_ && !table_filter_(row)) continue;
                const auto cell = table_format_cell_ ? table_format_cell_(row,i) :
                    TableCellFormat{.text = i < row.cells.size() ? row.cells[i] : std::string{}};
                width = std::max(width,textWidth(cell.text));
            }
            widths.push_back(std::max(1,table_columns_[i].width > 0 ? table_columns_[i].width : width+2));
        }
        const auto alignedX = [&](const int x, const int width, const int content_width,
            const TableAlignment alignment) {
            const int space = std::max(0,width-content_width);
            return x+(alignment == TableAlignment::Right ? space :
                alignment == TableAlignment::Center ? space/2 : 0);
        };
        const TextStyle heading {.foreground = palette.primary,.background = palette.surface,.bold = true};
        painter.fillBackground({.x = bounds_.x,.y = bounds_.y,.width = bounds_.width,.height = 1},palette.surface);
        int x = bounds_.x+1;
        for (std::size_t i = 0; i < widths.size(); ++i)
        {
            const std::string indicator = widths[i] >= 4 && table_sort_column_ == static_cast<int>(i) ?
                (table_sort_ascending_ ? " ▲" : " ▼") : "";
            const std::string label = clipText(table_columns_[i].title,
                std::max(0,widths[i]-1-textWidth(indicator)))+indicator;
            painter.text(alignedX(x,widths[i]-1,textWidth(label),table_columns_[i].alignment),
                bounds_.y,label,heading);
            x += widths[i]+1;
        }
        if (bounds_.height > 1)
            painter.horizontalLine(bounds_.x,bounds_.y+1,bounds_.width,
                {.foreground = palette.muted,.background = base});
        const int visible = std::max(0,bounds_.height-2);
        const auto order = tableOrder();
        const int scroll = table_scroll_.offset();
        for (int r = 0; r < visible && scroll+r < static_cast<int>(order.size()); ++r)
        {
            const auto& row = table_rows_[order[scroll+r]];
            const int y = bounds_.y+2+r;
            const bool selected = row.key == text_;
            const Color bg = selected ? blendColor(base,palette.primary,22) :
                r % 2 ? blendColor(base,palette.surface,35) : base;
            painter.fillBackground({.x = bounds_.x,.y = y,.width = bounds_.width,.height = 1},bg);
            const TextStyle style {.foreground = row.disabled ? palette.muted :
                selected ? palette.primary : palette.foreground,.background = bg,.bold = selected};
            int column_x = bounds_.x+1;
            for (std::size_t i = 0; i < widths.size(); ++i)
            {
                const auto formatted = table_format_cell_ ? table_format_cell_(row,i) :
                    TableCellFormat{.text = i < row.cells.size() ? row.cells[i] : std::string{}};
                TextStyle cell_style = style;
                if (!row.disabled)
                {
                    if (formatted.foreground) cell_style.foreground = *formatted.foreground;
                    if (formatted.background) cell_style.background = *formatted.background;
                    cell_style.bold = cell_style.bold || formatted.bold;
                }
                if (formatted.background && !row.disabled)
                    painter.fillBackground({.x = column_x,.y = y,.width = widths[i],.height = 1},
                        *formatted.background);
                const std::string cell_text = clipText(formatted.text,std::max(0,widths[i]-1));
                painter.text(alignedX(column_x,widths[i]-1,textWidth(cell_text),
                    table_columns_[i].alignment),y,cell_text,cell_style);
                column_x += widths[i]+1;
            }
        }
        const Rect track = scrollbarTrack();
        if (track.height > 0)
        {
            paintScrollbar(painter,track,scrollbarThumb(),theme ? *theme : Theme::dark(),
                palette.surface,unicode,scrollbar_drag_.active() ? ScrollbarInteraction::Active :
                    hovered_id == std::to_string(id_) ? ScrollbarInteraction::Hover : ScrollbarInteraction::Normal);
        }
        return;
    }
    if (type_ == Component::Type::Tabs)
    {
        if (bounds_.width <= 0 || bounds_.height <= 0) return;
        const Palette palette = theme ? theme->palette : Theme::dark().palette;
        const bool focused = focused_id == std::to_string(id_) && !disabled_;
        const bool hovered = hovered_id == std::to_string(id_) && !disabled_;
        const Color base = effective_background.value_or(palette.background);
        painter.fillBackground(bounds_,base);
        int x = bounds_.x;
        for (std::size_t i = 0; i < tab_options_.size(); ++i)
        {
            const auto& option = tab_options_[i];
            const int width = textWidth(option.label)+4;
            if (x >= bounds_.x+bounds_.width) break;
            const int visible_width = std::min(width,bounds_.x+bounds_.width-x);
            const bool selected = option.value == text_;
            const bool inactive = disabled_ || option.disabled;
            const Color tab_bg = selected || (hovered && hovered_tab_ == static_cast<int>(i) && !inactive) ?
                blendColor(base,palette.primary,selected ? 18 : 8) : base;
            const TextStyle tab_style {.foreground = inactive ? palette.muted :
                selected ? palette.primary : palette.foreground,.background = tab_bg,
                .bold = selected};
            painter.fillBackground({.x = x,.y = bounds_.y,.width = visible_width,
                .height = std::max(1,bounds_.height-1)},tab_bg);
            painter.text(x+2,bounds_.y+std::min(1,bounds_.height-1),
                clipText(option.label,std::max(0,visible_width-3)),tab_style);
            const TextStyle underline {.foreground = selected && !inactive ? palette.primary :
                focused && !inactive ? palette.muted : palette.surface,.background = base};
            painter.horizontalLine(x,bounds_.y+bounds_.height-1,visible_width,underline);
            x += width;
        }
        return;
    }
    if (type_ == Component::Type::Button)
    {
        if (bounds_.width <= 0 || bounds_.height <= 0) return;
        const Palette palette = theme ? theme->palette : Theme::dark().palette;
        const bool focused = focused_id == std::to_string(id_);
        const bool hovered = hovered_id == std::to_string(id_);
        const bool compact = bounds_.height < 3;
        const bool outlined = button_variant_ == ButtonVariant::Outlined;
        Color background_color = effective_background.value_or(palette.background);
        const Color accent = palette.resolve(color_variant_);
        Color foreground_color = accent;
        if (button_variant_ == ButtonVariant::Contained)
        {
            background_color = accent;
            foreground_color = palette.onColor(color_variant_);
        }
        if (disabled_)
        {
            background_color = palette.surface;
            foreground_color = palette.muted;
        }
        else if (hovered || focused)
        {
            const Color highlight = button_variant_ == ButtonVariant::Contained ?
                palette.onColor(color_variant_) : accent;
            background_color = blendColor(background_color,highlight,hovered ? 16 : 24);
        }
        if (button_variant_ == ButtonVariant::Text && !hovered && !focused && !disabled_)
            background_color = effective_background.value_or(palette.background);

        if (text_style_ != TextStyle{}) foreground_color = text_style_.foreground;
        const TextStyle button_style {.foreground = foreground_color,.background = background_color,
            .bold = focused || text_style_.bold,.underline = text_style_.underline};
        painter.fillBackground(bounds_,background_color);
        if (outlined)
        {
            if (compact)
            {
                painter.put(bounds_.x,bounds_.y,"▏",button_style);
                if (bounds_.width > 1) painter.put(bounds_.x+bounds_.width-1,bounds_.y,"▕",button_style);
            }
            else
                painter.border(bounds_,focused ? BorderStyle::Heavy : BorderStyle::Rounded,button_style);
        }
        const int inset = outlined ? 1 : 0;
        const int label_width = std::max(0,bounds_.width - inset * 2);
        const std::string label = clipText(text_,label_width);
        const int x = bounds_.x + inset + std::max(0,(label_width - textWidth(label)) / 2);
        const int y = bounds_.y + bounds_.height / 2;
        painter.text(x,y,label,button_style);
        return;
    }
    if (checkable())
    {
        if (bounds_.width <= 0 || bounds_.height <= 0) return;
        const Palette palette = theme ? theme->palette : Theme::dark().palette;
        const bool focused = focused_id == std::to_string(id_) && !disabled_;
        const bool hovered = hovered_id == std::to_string(id_) && !disabled_;
        const Color base = effective_background.value_or(palette.background);
        const Color accent = palette.resolve(color_variant_);
        const Color bg = hovered ? blendColor(base,accent,12) : base;
        painter.fillBackground(bounds_,bg);
        const auto draw = [&](const int y, const bool checked, const bool disabled,
            const bool radio, const std::string_view label) {
            const Color fg = disabled ? palette.muted : checked || focused ? accent : palette.foreground;
            const TextStyle indicator {.foreground = fg,.background = bg,.bold = focused};
            const TextStyle label_style {.foreground = disabled ? palette.muted : palette.foreground,.background = bg};
            std::string marker;
            if (!unicode)
                marker = type_ == Component::Type::Switch ? (checked ? "[ON]" : "[OFF]") :
                    radio ? (checked ? "(*)" : "( )") : (checked ? "[x]" : "[ ]");
            else if (type_ == Component::Type::Switch)
            {
                if (checkbox_variant_ == CheckboxVariant::Emoji) marker = checked ? "🟢" : "⚪";
                else if (checkbox_variant_ == CheckboxVariant::Unicode) marker = checked ? "◉━━" : "━━◯";
                else marker = checked ? "━━●" : "○━━";
            }
            else if (radio) marker = checked ? "◉" : "◯";
            else if (checkbox_variant_ == CheckboxVariant::Emoji) marker = checked ? "✅" : "⬜";
            else if (checkbox_variant_ == CheckboxVariant::Unicode) marker = checked ? "▣" : "▢";
            else marker = checked ? "■" : "□";
            painter.text(bounds_.x,y,marker,indicator);
            const int inset = type_ == Component::Type::Switch ? 6 :
                checkbox_variant_ == CheckboxVariant::Emoji && unicode ? 4 : 3;
            painter.text(bounds_.x+inset,y,clipText(label,std::max(0,bounds_.width-inset)),label_style);
        };
        if (type_ == Component::Type::RadioGroup)
        {
            for (int i = 0; i < static_cast<int>(radio_options_.size()) && i < bounds_.height; ++i)
                draw(bounds_.y+i,text_ == radio_options_[i].value,disabled_ || radio_options_[i].disabled,
                    true,radio_options_[i].label);
        }
        else draw(bounds_.y,checked_,disabled_,type_ == Component::Type::Radio,text_);
        return;
    }
    if (type_ == Component::Type::DropdownMenu)
    {
        if (bounds_.width <= 0 || bounds_.height <= 0) return;
        const Palette palette = theme ? theme->palette : Theme::dark().palette;
        const bool focused = focused_id == std::to_string(id_) || select_open_;
        const bool hovered = hovered_id == std::to_string(id_);
        const bool outlined = button_variant_ == ButtonVariant::Outlined;
        const bool contained = button_variant_ == ButtonVariant::Contained;
        const bool compact = bounds_.height < 3;
        Color bg = contained ? palette.primary : effective_background.value_or(palette.background);
        Color fg = contained ? palette.on_primary : palette.foreground;
        if (disabled_)
        {
            bg = palette.surface;
            fg = palette.muted;
        }
        else if (focused || hovered)
            bg = blendColor(bg,contained ? palette.on_primary : palette.primary,hovered ? 16 : 24);
        painter.fillBackground(bounds_,bg);
        TextStyle style {.foreground = fg,.background = bg,.bold = focused && !disabled_};
        if (outlined)
        {
            TextStyle outline = style;
            outline.foreground = disabled_ ? palette.muted : focused ? palette.primary : palette.muted;
            if (compact)
            {
                painter.put(bounds_.x,bounds_.y,"▏",outline);
                if (bounds_.width > 1) painter.put(bounds_.x+bounds_.width-1,bounds_.y,"▕",outline);
            }
            else painter.border(bounds_,focused ? BorderStyle::Heavy : BorderStyle::Rounded,outline);
        }
        const int inset = outlined ? 2 : 1;
        const int y = bounds_.y+bounds_.height/2;
        const int available = std::max(0,bounds_.width-inset*2-2);
        const std::string label = clipText(text_,available);
        painter.text(bounds_.x+inset,y,label,style);
        const int arrow_x = std::min(bounds_.x+bounds_.width-inset-1,
            bounds_.x+inset+textWidth(label)+1);
        if (arrow_x >= bounds_.x && arrow_x < bounds_.x+bounds_.width)
            painter.put(arrow_x,y,select_open_ ? "▴" : "▾",style);
        return;
    }
    if (type_ == Component::Type::Select)
    {
        if (bounds_.width <= 0 || bounds_.height <= 0) return;
        const Palette palette = theme ? theme->palette : Theme::dark().palette;
        const bool focused = focused_id == std::to_string(id_);
        const bool hovered = hovered_id == std::to_string(id_);
        const Color background_color = disabled_ ? palette.surface : hovered ?
            blendColor(palette.surface,palette.primary,12) : palette.surface;
        const TextStyle style {.foreground = disabled_ ? palette.muted : palette.foreground,
            .background = background_color};
        painter.fillBackground(bounds_,background_color);
        TextStyle outline = style;
        outline.foreground = disabled_ ? palette.muted : focused || select_open_ ? palette.primary :
            hovered ? blendColor(palette.muted,palette.primary,55) : palette.muted;
        if (bounds_.height >= 3) painter.border(bounds_,focused || select_open_ ? BorderStyle::Heavy : BorderStyle::Rounded,outline);
        else
        {
            painter.put(bounds_.x,bounds_.y,"▏",outline);
            if (bounds_.width > 1) painter.put(bounds_.x+bounds_.width-1,bounds_.y,"▕",outline);
        }
        std::string label = type_ == Component::Type::DropdownMenu ? text_ : placeholder_;
        if (type_ == Component::Type::Select)
            for (const auto& option : select_options_)
                if (option.value == text_) { label = option.label; break; }
        TextStyle label_style = style;
        if (label == placeholder_ && text_.empty()) label_style.foreground = palette.muted;
        painter.text(bounds_.x+2,bounds_.y+bounds_.height/2,clipText(label,std::max(0,bounds_.width-6)),label_style);
        if (bounds_.width >= 3) painter.put(bounds_.x+bounds_.width-3,bounds_.y+bounds_.height/2,
            select_open_ ? "▴" : "▾",style);
        return;
    }
    if (type_ == Component::Type::TextField)
    {
        if (bounds_.width <= 0 || bounds_.height <= 0) return;
        const Palette palette = theme ? theme->palette : Theme::dark().palette;
        const bool focused = focused_id == std::to_string(id_) && !disabled_;
        const bool hovered = hovered_id == std::to_string(id_) && !disabled_;
        const bool outlined = text_field_variant_ == TextFieldVariant::Outlined;
        const bool compact = bounds_.height < 3;
        Color background_color = disabled_ ? palette.surface :
            outlined ? effective_background.value_or(palette.background) : palette.surface;
        if (hovered && !focused) background_color = blendColor(background_color,palette.primary,10);
        const Color foreground_color = disabled_ ? palette.muted : palette.foreground;
        const TextStyle field_style {.foreground = foreground_color,.background = background_color};
        painter.fillBackground(bounds_,background_color);
        if (outlined)
        {
            TextStyle outline = field_style;
            outline.foreground = disabled_ ? palette.muted : focused ? palette.primary :
                hovered ? blendColor(palette.muted,palette.primary,55) : palette.muted;
            if (compact)
            {
                painter.put(bounds_.x,bounds_.y,"▏",outline);
                if (bounds_.width > 1) painter.put(bounds_.x+bounds_.width-1,bounds_.y,"▕",outline);
            }
            else painter.border(bounds_,focused ? BorderStyle::Heavy : BorderStyle::Rounded,outline);
        }
        else if ((focused || hovered) && bounds_.height > 1)
        {
            TextStyle underline = field_style;
            underline.foreground = focused ? palette.primary : blendColor(palette.muted,palette.primary,55);
            painter.horizontalLine(bounds_.x,bounds_.y+bounds_.height-1,bounds_.width,underline);
        }
        const int inset = outlined ? 2 : 1;
        const int columns = std::max(0,bounds_.width - inset * 2);
        const int cursor_columns = textWidth(std::string_view(text_).substr(0,cursor_byte_));
        scroll_columns_ = focused ? std::max(0,cursor_columns - std::max(0,columns - 1)) : 0;
        TextStyle content_style = field_style;
        if (text_.empty()) content_style.foreground = palette.muted;
        const std::string_view display = text_.empty() ? std::string_view(placeholder_) : std::string_view(text_);
        painter.text(bounds_.x+inset,bounds_.y+bounds_.height/2,
            clipWindow(display,scroll_columns_,columns),content_style);
        if (focused && columns > 0)
        {
            TextStyle caret_style = field_style;
            caret_style.foreground = palette.on_primary;
            caret_style.background = palette.primary;
            painter.styleAt(bounds_.x+inset+std::clamp(cursor_columns-scroll_columns_,0,columns-1),
                bounds_.y+bounds_.height/2,caret_style);
        }
        return;
    }
    if (type_ == Component::Type::Text)
    {
        if (bounds_.height > 0 && bounds_.width > 0)
        {
            painter.text(bounds_.x,bounds_.y,clipText(text_,bounds_.width),style);
        }
        return;
    }
    if (type_ == Component::Type::ResizableSplit)
    {
        for (const auto& child : children_)
        {
            Painter::ClipScope clip(painter,child->bounds_);
            child->paint(painter,focused_id,hovered_id,effective_background,theme,unicode);
        }
        const Rect handle = splitHandle();
        if (handle.width > 0 && handle.height > 0)
        {
            const Palette palette = theme ? theme->palette : Theme::dark().palette;
            const bool focused = focused_id == std::to_string(id_);
            const TextStyle divider {.foreground = focused ? palette.primary : palette.muted,
                .background = effective_background.value_or(palette.background),.bold = focused};
            if (split_direction_ == SplitDirection::Horizontal)
                painter.verticalLine(handle.x,handle.y,handle.height,divider);
            else painter.horizontalLine(handle.x,handle.y,handle.width,divider);
        }
        return;
    }
    for (const auto& child : children_) child->paint(painter,focused_id,hovered_id,effective_background,theme,unicode);
    if (const Rect track = scrollbarTrack(); track.height > 0)
    {
        const Palette palette = theme ? theme->palette : Theme::dark().palette;
        paintScrollbar(painter,track,scrollbarThumb(),theme ? *theme : Theme::dark(),
            effective_background.value_or(palette.background),unicode,
            scrollbar_drag_.active() ? ScrollbarInteraction::Active :
                hovered_id == std::to_string(id_) ? ScrollbarInteraction::Hover : ScrollbarInteraction::Normal);
    }
    if (border_) painter.border(bounds_,*border_,style);
}

int RenderNode::height() const noexcept
{
    return bounds_.height;
}

