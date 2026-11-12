/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <terion/render_node.h>
#include <algorithm>

using namespace terion;

bool RenderNode::handleVirtualListKey(const Event& event)
{
    if (type_ != Component::Type::VirtualList || disabled_ || virtual_item_count_ <= 0)
        return false;

    const int page = std::max(1,virtual_scroll_.viewport());
    const int current = virtual_selected_index_ >= 0 && virtual_selected_index_ < virtual_item_count_ ?
        virtual_selected_index_ : -1;
    long long next = current;
    switch (event.key)
    {
        case Key::Up: next = current < 0 ? virtual_item_count_-1 : next-1; break;
        case Key::Down: next = current < 0 ? 0 : next+1; break;
        case Key::PageUp: next = current < 0 ? 0 : next-page; break;
        case Key::PageDown: next = current < 0 ? 0 : next+page; break;
        case Key::Home: next = 0; break;
        case Key::End: next = virtual_item_count_-1; break;
        default: return false;
    }
    next = std::clamp(next,0LL,static_cast<long long>(virtual_item_count_-1));
    const int index = static_cast<int>(next);
    virtual_scroll_.revealItem(index);
    if (index != current && virtual_on_select_) virtual_on_select_(index);
    return true;
}

bool RenderNode::handleVirtualListClick(const Point position)
{
    if (type_ != Component::Type::VirtualList || disabled_ ||
        position.x < bounds_.x || position.y < bounds_.y ||
        position.x >= bounds_.x+bounds_.width || position.y >= bounds_.y+bounds_.height ||
        (scrollbarTrack().width > 0 && position.x == scrollbarTrack().x)) return false;
    const int index = virtual_scroll_.offset()+position.y-bounds_.y;
    if (index < 0 || index >= virtual_item_count_) return false;
    if (index != virtual_selected_index_ && virtual_on_select_) virtual_on_select_(index);
    return true;
}
