/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <terion/render_node.h>
#include <terion/utf8.h>
#include <algorithm>
#include <limits>

using namespace terion;

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

std::size_t previousCodepoint(const std::string_view text, const std::size_t cursor)
{
    std::size_t previous = 0;
    for (std::size_t i = 0; i < cursor;)
    {
        const auto decoded = decodeUtf8(text,i);
        if (!decoded.bytes || i + decoded.bytes >= cursor) return i;
        previous = i;
        i += decoded.bytes;
    }
    return previous;
}

std::size_t nextCodepoint(const std::string_view text, const std::size_t cursor)
{
    if (cursor >= text.size()) return text.size();
    const auto decoded = decodeUtf8(text,cursor);
    return std::min(text.size(),cursor + std::max<std::size_t>(1,decoded.bytes));
}

std::size_t previousBoundary(const std::string_view text, const std::size_t cursor)
{
    std::size_t start = previousCodepoint(text,cursor);
    while (start > 0 && codePointWidth(decodeUtf8(text,start).code_point) == 0)
        start = previousCodepoint(text,start);
    return start;
}

std::size_t nextBoundary(const std::string_view text, const std::size_t cursor)
{
    std::size_t end = nextCodepoint(text,cursor);
    while (end < text.size() && codePointWidth(decodeUtf8(text,end).code_point) == 0)
        end = nextCodepoint(text,end);
    return end;
}

std::string encodeUtf8(const char32_t code_point)
{
    const char32_t c = code_point > 0x10FFFF || (code_point >= 0xD800 && code_point <= 0xDFFF) ? 0xFFFD : code_point;
    std::string result;
    if (c < 0x80) result += static_cast<char>(c);
    else if (c < 0x800)
    {
        result += static_cast<char>(0xC0 | (c >> 6));
        result += static_cast<char>(0x80 | (c & 0x3F));
    }
    else if (c < 0x10000)
    {
        result += static_cast<char>(0xE0 | (c >> 12));
        result += static_cast<char>(0x80 | ((c >> 6) & 0x3F));
        result += static_cast<char>(0x80 | (c & 0x3F));
    }
    else
    {
        result += static_cast<char>(0xF0 | (c >> 18));
        result += static_cast<char>(0x80 | ((c >> 12) & 0x3F));
        result += static_cast<char>(0x80 | ((c >> 6) & 0x3F));
        result += static_cast<char>(0x80 | (c & 0x3F));
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

} /* End of anonymous namespace */

Size RenderNode::measure(const Constraints& constraints) const
{
    const int padding = std::max(0,style_.padding) + (border_ ? 1 : 0);
    const int gap = std::max(0,style_.gap);
    Size content{};

    if (type_ == Component::Type::Divider)
    {
        content = {.width = constraints.maximum.width,.height = 1};
    }
    else if (type_ == Component::Type::Progress)
    {
        content = {.width = 20,.height = 1};
    }
    else if (type_ == Component::Type::Button)
    {
        const int padding = button_size_ == ButtonSize::Large ? 3 : button_size_ == ButtonSize::Compact ? 1 : 2;
        content = {.width = addBounded(textWidth(text_),padding * 2 +
            (button_variant_ == ButtonVariant::Outlined ? 2 : 0)),
            .height = button_size_ == ButtonSize::Compact ? 1 : button_size_ == ButtonSize::Large ? 5 : 3};
    }
    else if (type_ == Component::Type::TextField)
    {
        const int padding = button_size_ == ButtonSize::Large ? 3 : button_size_ == ButtonSize::Compact ? 1 : 2;
        const int content_width = std::max(textWidth(text_),textWidth(placeholder_));
        content = {.width = addBounded(content_width,padding * 2 +
            (text_field_variant_ == TextFieldVariant::Outlined ? 2 : 0)),
            .height = button_size_ == ButtonSize::Compact ? 1 : button_size_ == ButtonSize::Large ? 5 : 3};
    }
    else if (type_ == Component::Type::Text)
    {
        content = {.width = textWidth(text_),.height = 1};
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
        for (std::size_t i = 0; i < children_.size(); ++i)
        {
            const auto& child = children_[i];
            const auto child_size = child->measure(Constraints{.maximum = max_content});
            if (type_ == Component::Type::Row)
            {
                if (i > 0) content.width = addBounded(content.width,gap);
                content.width = addBounded(content.width,child_size.width);
                content.height = std::max(content.height,child_size.height);
            }
            else
            {
                if (i > 0) content.height = addBounded(content.height,gap);
                content.height = addBounded(content.height,child_size.height);
                content.width = std::max(content.width,child_size.width);
            }
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
    if (type_ == Component::Type::Text || type_ == Component::Type::Button ||
        type_ == Component::Type::TextField || type_ == Component::Type::Divider ||
        type_ == Component::Type::Progress) return;

    const int padding = std::max(0,style_.padding) + (border_ ? 1 : 0);
    const int gap = std::max(0,style_.gap);
    const int inset = addBounded(padding,padding);
    const int inner_width = std::max(0,size.width - inset);
    const int inner_height = std::max(0,size.height - inset);
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
        const int gaps = children_.empty() ? 0 : static_cast<int>(children_.size() - 1);
        const int gap_space = static_cast<int>(std::min(static_cast<long long>(main_extent),
            static_cast<long long>(gaps) * gap));
        std::vector<int> sizes(children_.size(),0);
        int fixed_extent = 0;
        long long total_flex = 0;
        for (std::size_t i = 0; i < children_.size(); ++i)
        {
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
        for (std::size_t i = 0; i < children_.size(); ++i)
        {
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
            cursor += sizes[i] + gap;
            if (i + 1 < children_.size())
            {
                cursor += between;
                if (remainder > 0) { ++cursor; --remainder; }
            }
        }
        return;
    }
    int x = origin.x + padding;
    int y = origin.y + padding;
    for (const auto& child : children_)
    {
        const bool horizontal = type_ == Component::Type::Row;
        const Size available {.width = horizontal ? std::max(0,inner_width - (x - origin.x - padding)) : inner_width,
            .height = horizontal ? inner_height : std::max(0,inner_height - (y - origin.y - padding))};
        child->layout(Constraints{.maximum = available},{.x = x,.y = y});
        if (horizontal) x += child->bounds_.width + gap;
        else y += child->bounds_.height + gap;
    }
}

const RenderNode* RenderNode::hitTest(const Point position) const noexcept
{
    if (position.x < bounds_.x || position.y < bounds_.y ||
        position.x >= bounds_.x + bounds_.width || position.y >= bounds_.y + bounds_.height) return nullptr;
    for (auto it = children_.rbegin(); it != children_.rend(); ++it)
        if (const auto* hit = (*it)->hitTest(position)) return hit;
    return this;
}

void RenderNode::paint(Painter& painter, const std::string_view focused_id,
    const std::string_view hovered_id, const std::optional<Color> background, const Theme* theme) const
{
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
        const int filled = static_cast<int>(bounds_.width * std::clamp(value_,0.0,1.0));
        for (int i = 0; i < bounds_.width; ++i)
        {
            TextStyle segment_style = style;
            if (theme) segment_style.foreground = i < filled ? theme->palette.primary : theme->palette.muted;
            painter.put(bounds_.x+i,bounds_.y,i < filled ? "█" : "░",segment_style);
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
        Color foreground_color = palette.primary;
        if (button_variant_ == ButtonVariant::Contained)
        {
            background_color = palette.primary;
            foreground_color = palette.on_primary;
        }
        if (disabled_)
        {
            background_color = palette.surface;
            foreground_color = palette.muted;
        }
        else if (hovered || focused)
        {
            const Color highlight = button_variant_ == ButtonVariant::Contained ?
                palette.on_primary : palette.primary;
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
    if (type_ == Component::Type::TextField)
    {
        if (bounds_.width <= 0 || bounds_.height <= 0) return;
        const Palette palette = theme ? theme->palette : Theme::dark().palette;
        const bool focused = focused_id == std::to_string(id_) && !disabled_;
        const bool outlined = text_field_variant_ == TextFieldVariant::Outlined;
        const bool compact = bounds_.height < 3;
        const Color background_color = disabled_ ? palette.surface :
            outlined ? effective_background.value_or(palette.background) : palette.surface;
        const Color foreground_color = disabled_ ? palette.muted : palette.foreground;
        const TextStyle field_style {.foreground = foreground_color,.background = background_color};
        painter.fillBackground(bounds_,background_color);
        if (outlined)
        {
            TextStyle outline = field_style;
            outline.foreground = disabled_ ? palette.muted : focused ? palette.primary : palette.muted;
            if (compact)
            {
                painter.put(bounds_.x,bounds_.y,"▏",outline);
                if (bounds_.width > 1) painter.put(bounds_.x+bounds_.width-1,bounds_.y,"▕",outline);
            }
            else painter.border(bounds_,focused ? BorderStyle::Heavy : BorderStyle::Rounded,outline);
        }
        else if (focused && bounds_.height > 1)
        {
            TextStyle underline = field_style;
            underline.foreground = palette.primary;
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
    for (const auto& child : children_) child->paint(painter,focused_id,hovered_id,effective_background,theme);
    if (border_) painter.border(bounds_,*border_,style);
}

int RenderNode::height() const noexcept
{
    return bounds_.height;
}

bool RenderNode::handleTextInput(const Event& event)
{
    if (type_ != Component::Type::TextField || !editable()) return false;
    switch (event.key)
    {
        case Key::Left: cursor_byte_ = previousBoundary(text_,cursor_byte_); return true;
        case Key::Right: cursor_byte_ = nextBoundary(text_,cursor_byte_); return true;
        case Key::Home: cursor_byte_ = 0; return true;
        case Key::End: cursor_byte_ = text_.size(); return true;
        case Key::Backspace:
            if (cursor_byte_ == 0) return true;
            {
                const auto previous = previousBoundary(text_,cursor_byte_);
                text_.erase(previous,cursor_byte_ - previous);
                cursor_byte_ = previous;
                on_change_(text_);
                return true;
            }
        case Key::Delete:
            if (cursor_byte_ == text_.size()) return true;
            text_.erase(cursor_byte_,nextBoundary(text_,cursor_byte_) - cursor_byte_);
            on_change_(text_);
            return true;
        case Key::Character:
            if (event.character < 32 || event.character == 127) return false;
            {
                const auto encoded = encodeUtf8(event.character);
                text_.insert(cursor_byte_,encoded);
                cursor_byte_ += encoded.size();
                on_change_(text_);
                return true;
            }
        default: return false;
    }
}

void RenderNode::setCursorFromX(const int x)
{
    if (type_ != Component::Type::TextField) return;
    const int inset = text_field_variant_ == TextFieldVariant::Outlined ? 2 : 1;
    const int desired = std::max(0,x - bounds_.x - inset + scroll_columns_);
    int columns = 0;
    cursor_byte_ = 0;
    while (cursor_byte_ < text_.size())
    {
        const auto decoded = decodeUtf8(text_,cursor_byte_);
        if (!decoded.bytes || columns + codePointWidth(decoded.code_point) > desired) break;
        columns += codePointWidth(decoded.code_point);
        cursor_byte_ += decoded.bytes;
    }
}
