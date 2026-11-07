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
    else if (type_ == Component::Type::Text || type_ == Component::Type::Button || type_ == Component::Type::TextField)
    {
        const auto& value = type_ == Component::Type::TextField && text_.empty() ? placeholder_ : text_;
        content = {.width = addBounded(textWidth(value),type_ == Component::Type::Text ? 0 :
            type_ == Component::Type::TextField ? 3 : button_variant_ == ButtonVariant::Text ? 0 : 2),.height = 1};
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
    const auto effective_background = background_.has_value() ? background_ : background;
    TextStyle style = text_style_;
    if (theme && text_style_ == TextStyle{})
    {
        style.foreground = theme->palette.foreground;
        style.background = theme->palette.background;
    }
    if (effective_background) style.background = *effective_background;

    if (background_) painter.fillBackground(bounds_,*background_);
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
    if (type_ == Component::Type::Text || type_ == Component::Type::Button ||
        type_ == Component::Type::TextField)
    {
        if (bounds_.height > 0 && bounds_.width > 0)
        {
            std::string label = text_;
            if (type_ == Component::Type::Button)
            {
                if (theme && button_variant_ == ButtonVariant::Contained)
                {
                    style.foreground = theme->palette.on_primary;
                    style.background = theme->palette.primary;
                }
                else if (theme && button_variant_ == ButtonVariant::Outlined)
                    style.foreground = theme->palette.primary;
                if (disabled_) label = "(" + label + ")";
                else if (button_variant_ == ButtonVariant::Text) {}
                else if (focused_id == std::to_string(id_)) label = ">" + label + "<";
                else if (hovered_id == std::to_string(id_)) label = "{" + label + "}";
                else if (button_variant_ == ButtonVariant::Contained) label = " " + label + " ";
                else label = "[" + label + "]";
            }
            else if (type_ == Component::Type::TextField)
            {
                if (text_.empty()) label = placeholder_;
                if (focused_id == std::to_string(id_) && !disabled_)
                    label.insert(std::min(cursor_byte_,label.size()),"|");
                label = disabled_ ? "(" + label + ")" : "[" + label + "]";
            }
            painter.text(bounds_.x,bounds_.y,clipText(label,bounds_.width),style);
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
    const int desired = std::max(0,x - bounds_.x - 1);
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
