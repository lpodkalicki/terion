/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#ifndef TERION_LAYOUT_H
#define TERION_LAYOUT_H

#include <algorithm>
#include <limits>
#include <optional>
#include <terion/geometry.h>

namespace terion {

enum class JustifyContent
{
    Start,
    Center,
    End,
    SpaceBetween
}; /* End of enum JustifyContent */

enum class AlignItems
{
    Start,
    Center,
    End,
    Stretch
}; /* End of enum AlignItems */

struct Constraints
{
    Size minimum{};
    Size maximum {.width = std::numeric_limits<int>::max(),.height = std::numeric_limits<int>::max()};

    [[nodiscard]] Size constrain(const Size size) const noexcept
    {
        const int max_width = std::max(0,maximum.width);
        const int max_height = std::max(0,maximum.height);
        return {.width = std::clamp(size.width,std::clamp(minimum.width,0,max_width),max_width),
            .height = std::clamp(size.height,std::clamp(minimum.height,0,max_height),max_height)};
    }
}; /* End of struct Constraints */

struct LayoutStyle
{
    std::optional<int> width = std::nullopt;
    std::optional<int> height = std::nullopt;
    int padding = 0;
    int gap = 0;
    std::optional<int> minWidth = std::nullopt;
    std::optional<int> maxWidth = std::nullopt;
    std::optional<int> minHeight = std::nullopt;
    std::optional<int> maxHeight = std::nullopt;
    int flex = 0;
    JustifyContent justifyContent = JustifyContent::Start;
    AlignItems alignItems = AlignItems::Start;

    bool operator==(const LayoutStyle&) const = default;
}; /* End of struct LayoutStyle */

} /* End of namespace terion */

#endif /* TERION_LAYOUT_H */
