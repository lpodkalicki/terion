/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#ifndef TERION_EMOJI_COMPAT_H
#define TERION_EMOJI_COMPAT_H

#include <string>
#include <string_view>
#include <terion/grapheme.h>

namespace terion {

// Some terminals render ZWJ emoji as separate pictographs, skin modifiers as
// separate glyphs, and VS16 hearts as one column. Their *physical* width then
// differs from the logical grapheme width used throughout the layout engine.
// Keep the original grapheme in ScreenBuffer and only substitute the bytes
// sent to a terminal configured for legacy emoji rendering.
inline std::string compatibleTerminalGrapheme(std::string_view value, int reserved_columns)
{
    bool pictographic = false;
    bool has_zwj = false;
    bool has_modifier = false;
    bool has_vs16 = false;
    std::size_t first_bytes = 0;
    for (std::size_t i = 0; i < value.size();)
    {
        const auto decoded = decodeUtf8(value,i);
        if (!decoded.bytes) break;
        if (i == 0) first_bytes = decoded.bytes;
        const auto c = decoded.code_point;
        pictographic |= unicode_detail::pictographic(c);
        has_zwj |= c == 0x200D;
        has_modifier |= unicode_detail::in(c,0x1F3FB,0x1F3FF);
        has_vs16 |= c == 0xFE0F;
        i += decoded.bytes;
    }
    if (!pictographic || !(has_zwj || has_modifier || has_vs16) || first_bytes == 0)
        return std::string(value);

    // Display only the first base pictograph. In legacy terminals it has a
    // stable width unlike the composition that contains ZWJ/modifier/VS16.
    // For narrow symbols (e.g. U+2764 HEART), pad the remaining logical cell.
    std::string replacement(value.substr(0,first_bytes));
    const int width = displayWidth(replacement);
    if (width <= 0 || width > reserved_columns)
        return std::string(static_cast<std::size_t>(reserved_columns),'?');
    replacement.append(static_cast<std::size_t>(reserved_columns-width),' ');
    return replacement;
}

} /* End of namespace terion */

#endif /* TERION_EMOJI_COMPAT_H */
