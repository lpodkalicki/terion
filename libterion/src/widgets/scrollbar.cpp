/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <terion/render_node.h>
#include <algorithm>

using namespace terion;

const ScrollController& RenderNode::scrollController() const noexcept
{
    if (type_ == Component::Type::Dialog) return dialog_scroll_;
    if (type_ == Component::Type::TreeView) return tree_scroll_;
    if (type_ == Component::Type::Table) return table_scroll_;
    return scroll_;
}

Rect RenderNode::scrollbarTrack() const
{
    if (type_ == Component::Type::Dialog)
    {
        if (!dialog_open() || !dialog_scroll_.scrollable()) return {};
        return {.x = dialog_content_rect_.x+dialog_content_rect_.width,
            .y = dialog_content_rect_.y,.width = 1,.height = dialog_content_rect_.height};
    }
    if (type_ == Component::Type::TreeView)
    {
        if (bounds_.width <= 0 || bounds_.height <= 0 ||
            static_cast<int>(tree_visible_items().size()) <= bounds_.height) return {};
        return {.x = bounds_.x+bounds_.width-1,.y = bounds_.y,.width = 1,.height = bounds_.height};
    }
    if (type_ == Component::Type::Table)
    {
        const int height = std::max(0,bounds_.height-2);
        if (bounds_.width <= 0 || height <= 0 ||
            static_cast<int>(table_order().size()) <= height) return {};
        return {.x = bounds_.x+bounds_.width-1,.y = bounds_.y+2,.width = 1,.height = height};
    }
    if (type_ != Component::Type::ScrollView || !scrollbar_visible_ || !scroll_.scrollable() ||
        bounds_.width <= 0) return {};
    const int inset = std::max(0,style_.padding) + (border_ ? 1 : 0);
    return {.x = bounds_.x+bounds_.width-inset-1,
        .y = bounds_.y+inset,.width = 1,.height = scroll_.viewport()};
}

Rect RenderNode::scrollbarThumb() const
{
    return scrollController().thumb(scrollbarTrack());
}

bool RenderNode::scrollbar_drag(const int y, const int grab_offset)
{
    if (type_ != Component::Type::Dialog && type_ != Component::Type::Table &&
        type_ != Component::Type::TreeView && !on_scroll_) return false;
    const Rect track = scrollbarTrack();
    const ScrollController scroll = scrollController();
    if (!scroll.scrollable() || track.height <= 0) return false;
    applyScrollOffset(scroll.offsetAt(track,y,grab_offset));
    return true;
}

void RenderNode::applyScrollOffset(const int target)
{
    const ScrollController scroll = scrollController();
    if (type_ == Component::Type::Dialog)
        (void)scrollDialog(target-dialog_scroll_.offset(),dialog_screen_);
    else if (type_ == Component::Type::Table) table_scroll_.setOffset(target);
    else if (type_ == Component::Type::TreeView) tree_scroll_.setOffset(target);
    else if (on_scroll_ && target != scroll.offset()) on_scroll_(target);
}

bool RenderNode::scrollbarPress(const Point position, int& grab_offset)
{
    const Rect track = scrollbarTrack();
    if (type_ != Component::Type::Dialog && type_ != Component::Type::Table &&
        type_ != Component::Type::TreeView && !on_scroll_) return false;
    int target = 0;
    if (!scrollController().press(track,position,grab_offset,target)) return false;
    applyScrollOffset(target);
    return true;
}

bool RenderNode::beginScrollbarDrag(const Point position)
{
    if (type_ != Component::Type::Dialog && type_ != Component::Type::Table &&
        type_ != Component::Type::TreeView && !on_scroll_) return false;
    int target = 0;
    if (!scrollbar_drag_.begin(scrollController(),scrollbarTrack(),position,target)) return false;
    applyScrollOffset(target);
    return true;
}

bool RenderNode::moveScrollbarDrag(const int y)
{
    if (!scrollbar_drag_.active()) return false;
    applyScrollOffset(scrollbar_drag_.move(scrollController(),scrollbarTrack(),y));
    return true;
}
