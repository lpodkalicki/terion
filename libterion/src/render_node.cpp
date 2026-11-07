/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <terion/render_node.h>

using namespace terion;

int RenderNode::height() const noexcept
{
    if (type_ == Component::Type::Text) return 1;
    int total = 0;
    for (const auto& child : children_) total += child->height();
    return total;
}

void RenderNode::paint(Painter& painter, const Point origin) const
{
    if (type_ == Component::Type::Text)
    {
        painter.text(origin.x,origin.y,text_);
        return;
    }

    int y = origin.y;
    for (const auto& child : children_)
    {
        child->paint(painter,{.x = origin.x,.y = y});
        y += child->height();
    }
}
