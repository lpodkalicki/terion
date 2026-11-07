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

int addBounded(const int a, const int b)
{
    return static_cast<int>(std::min(static_cast<long long>(a) + b,
        static_cast<long long>(std::numeric_limits<int>::max())));
}

} /* End of anonymous namespace */

Size RenderNode::measure(const Constraints& constraints) const
{
    const int padding = std::max(0,style_.padding);
    const int gap = std::max(0,style_.gap);
    Size content{};

    if (type_ == Component::Type::Text || type_ == Component::Type::Button)
    {
        content = {.width = addBounded(textWidth(text_),type_ == Component::Type::Button ? 2 : 0),.height = 1};
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
    if (type_ == Component::Type::Text || type_ == Component::Type::Button) return;

    const int padding = std::max(0,style_.padding);
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
    const std::string_view hovered_id) const
{
    if (type_ == Component::Type::Text || type_ == Component::Type::Button)
    {
        if (bounds_.height > 0 && bounds_.width > 0)
        {
            std::string label = text_;
            if (type_ == Component::Type::Button)
            {
                if (disabled_) label = "(" + label + ")";
                else if (focused_id == std::to_string(id_)) label = ">" + label + "<";
                else if (hovered_id == std::to_string(id_)) label = "{" + label + "}";
                else label = "[" + label + "]";
            }
            painter.text(bounds_.x,bounds_.y,clipText(label,bounds_.width),text_style_);
        }
        return;
    }
    for (const auto& child : children_) child->paint(painter,focused_id,hovered_id);
}

int RenderNode::height() const noexcept
{
    return bounds_.height;
}
