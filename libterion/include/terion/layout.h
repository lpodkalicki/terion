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

// Computes a safe number of columns without overflowing on large constraints.
// A min_column_width of zero disables adaptive layout (the legacy behavior).
[[nodiscard]] constexpr int responsiveColumns(int available_width, int requested_columns,
    int gap, int min_column_width) noexcept
{
    const int requested = std::max(1,requested_columns);
    if (min_column_width <= 0) return requested;
    const long long space = std::max(0,available_width);
    const long long separation = std::max(0,gap);
    const long long minimum = std::max(1,min_column_width);
    const long long count = (space+separation)/(minimum+separation);
    return static_cast<int>(std::clamp(count,1LL,static_cast<long long>(requested)));
}

// Explicit design tokens override literal padding/gap values when requested.
// Unspecified preserves the old per-component layout behavior.
enum class SpacingToken { Unspecified, None, Small, Medium, Large };

struct LayoutStyle
{
    std::optional<int> width = std::nullopt;
    std::optional<int> height = std::nullopt;
    int padding = 0;
    int gap = 0;
    std::optional<int> min_width = std::nullopt;
    std::optional<int> max_width = std::nullopt;
    std::optional<int> min_height = std::nullopt;
    std::optional<int> max_height = std::nullopt;
    int flex = 0;
    JustifyContent justify_content = JustifyContent::Start;
    AlignItems align_items = AlignItems::Start;
    bool clip = false;
    int grid_min_column_width = 0; // Grid only; zero retains fixed columns.
    SpacingToken padding_token = SpacingToken::Unspecified;
    SpacingToken gap_token = SpacingToken::Unspecified;

    bool operator==(const LayoutStyle&) const = default;
}; /* End of struct LayoutStyle */

} /* End of namespace terion */

#endif /* TERION_LAYOUT_H */
