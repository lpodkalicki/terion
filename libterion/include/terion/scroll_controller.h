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
    ScrollController(int content, int viewport, int offset = 0) noexcept :
        viewport_(std::max(0,viewport)),
        maximum_(static_cast<int>(std::max(0LL,static_cast<long long>(content)-viewport_))),
        offset_(clamp(offset))
    {}

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
        const int top = static_cast<int>((static_cast<long long>(offset_)*travel+maximum_/2)/maximum_);
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

private:
    int viewport_ = 0;
    int maximum_ = 0;
    int offset_ = 0;
}; /* End of class ScrollController */

} /* End of namespace terion */

#endif /* TERION_SCROLL_CONTROLLER_H */
