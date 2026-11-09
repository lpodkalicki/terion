/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#ifndef TERION_SCROLL_CONTROLLER_H
#define TERION_SCROLL_CONTROLLER_H

#include <algorithm>
#include <cstdint>
#include <terion/layout.h>

namespace terion {

// Scroll extents are expressed in the caller's units (rows or list items).
// The controller owns scroll math; widgets decide how to persist the offset.
class ScrollController
{
public:
    ScrollController() noexcept = default;
    ScrollController(int content, int viewport, int offset = 0) noexcept :
        viewport_(std::max(0,viewport)),
        maximum_(static_cast<int>(std::max(0LL,static_cast<long long>(content)-viewport_))),
        offset_(clamp(offset))
    {}

    void configure(int content, int viewport) noexcept
    {
        viewport_ = std::max(0,viewport);
        maximum_ = static_cast<int>(std::max(0LL,static_cast<long long>(content)-viewport_));
        offset_ = clamp(offset_);
    }

    void setOffset(int offset) noexcept { offset_ = clamp(offset); }

    // Used by controlled components before their next layout measures content.
    void setRequestedOffset(int offset) noexcept { offset_ = std::max(0,offset); }

    [[nodiscard]] int offset() const noexcept { return offset_; }
    [[nodiscard]] int maximum() const noexcept { return maximum_; }
    [[nodiscard]] int viewport() const noexcept { return viewport_; }
    [[nodiscard]] bool scrollable() const noexcept { return maximum_ > 0 && viewport_ > 0; }

    [[nodiscard]] int clamp(long long value) const noexcept
    {
        return static_cast<int>(std::clamp(value,0LL,static_cast<long long>(maximum_)));
    }

    [[nodiscard]] int scrollBy(int delta) const noexcept
    {
        return clamp(static_cast<long long>(offset_)+delta);
    }

    void advance(int delta) noexcept { offset_ = scrollBy(delta); }

    void revealItem(int first, int length = 1) noexcept { offset_ = reveal(first,length); }

    [[nodiscard]] int reveal(int first, int length = 1) const noexcept
    {
        if (viewport_ <= 0) return offset_;
        const long long last = static_cast<long long>(first)+std::max(1,length);
        if (first < offset_) return clamp(first);
        if (last > static_cast<long long>(offset_)+viewport_)
            return clamp(last-viewport_);
        return offset_;
    }

    [[nodiscard]] Rect thumb(Rect track) const noexcept
    {
        if (!scrollable() || track.height <= 0) return {};
        const long long extent = static_cast<long long>(maximum_)+viewport_;
        const int height = std::max(1,static_cast<int>(static_cast<long long>(track.height)*
            viewport_/extent));
        const int travel = std::max(0,track.height-height);
        // With only two positions, show progress as soon as half the content
        // has been traversed. Larger tracks retain the stable floor mapping.
        const long long numerator = static_cast<long long>(offset_)*travel +
            (travel == 1 ? maximum_/2 : 0);
        const int top = static_cast<int>(numerator/maximum_);
        return {.x = track.x,.y = track.y+top,.width = track.width,.height = height};
    }

    [[nodiscard]] int offsetAt(Rect track, int y, int grab_offset) const noexcept
    {
        const Rect handle = thumb(track);
        const int travel = std::max(0,track.height-handle.height);
        if (travel <= 0) return 0;
        const long long position = std::clamp(static_cast<long long>(y)-track.y-grab_offset,
            0LL,static_cast<long long>(travel));
        return clamp((position*maximum_+travel/2)/travel);
    }

    [[nodiscard]] bool press(Rect track, Point point, int& grab_offset, int& target) const noexcept
    {
        if (!scrollable() || track.width <= 0 || track.height <= 0 ||
            point.x < track.x || point.x >= track.x+track.width ||
            point.y < track.y || point.y >= track.y+track.height) return false;
        const Rect handle = thumb(track);
        const bool inside_handle = point.y >= handle.y && point.y < handle.y+handle.height;
        grab_offset = inside_handle ? point.y-handle.y : handle.height/2;
        target = inside_handle ? offset_ : offsetAt(track,point.y,grab_offset);
        return true;
    }

    // Mouse capture belongs to the widget, independently of its scroll offset.
    class DragState
    {
    public:
        [[nodiscard]] bool begin(const ScrollController& scroll, Rect track, Point point,
            int& target) noexcept
        {
            end();
            active_ = scroll.press(track,point,grab_offset_,target);
            return active_;
        }

        [[nodiscard]] int move(const ScrollController& scroll, Rect track, int y) const noexcept
        {
            return active_ ? scroll.offsetAt(track,y,grab_offset_) : scroll.offset();
        }

        void end() noexcept { active_ = false; grab_offset_ = 0; }
        [[nodiscard]] bool active() const noexcept { return active_; }

    private:
        bool active_ = false;
        int grab_offset_ = 0;
    }; /* End of class DragState */

private:
    int viewport_ = 0;
    int maximum_ = 0;
    int offset_ = 0;
}; /* End of class ScrollController */

} /* End of namespace terion */

#endif /* TERION_SCROLL_CONTROLLER_H */
