/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#ifndef TERION_RENDER_COLOR_H
#define TERION_RENDER_COLOR_H

#include <cstdint>

namespace terion {

enum class BorderStyle
{
    Single,
    Double,
    Rounded,
    Heavy
}; /* End of enum BorderStyle */

struct Color
{
    std::uint8_t red = 255, green = 255, blue = 255;
    bool operator==(const Color&) const = default;
    static constexpr Color rgb(const std::uint8_t red, const std::uint8_t green, const std::uint8_t blue)
    {
        return {.red = red,.green = green,.blue = blue};
    }
}; /* End of struct Color */

struct TextStyle
{
    Color foreground = {.red = 255,.green = 255,.blue = 255};
    Color background = {.red = 0,.green = 0,.blue = 0};
    bool bold = false;
    bool underline = false;
    bool italic = false;
    bool strikethrough = false;
    bool dim = false;
    bool operator==(const TextStyle&) const = default;
}; /* End of struct TextStyle */

} /* End of namespace terion */

#endif /* TERION_RENDER_COLOR_H */
