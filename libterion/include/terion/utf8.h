/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#ifndef TERION_UTF8_H
#define TERION_UTF8_H

#include <cstdint>
#include <string_view>

namespace terion {

struct Decoded
{
    char32_t code_point;
    std::size_t bytes;
}; /* End of struct Decoded */

// Invalid sequences yield U+FFFD and consume one byte.
inline Decoded decodeUtf8(const std::string_view s, const std::size_t pos)
{
    if (pos >= s.size()) return {.code_point = 0, .bytes = 0};
    auto b = [&](const std::size_t i) { return static_cast<std::uint8_t>(s[pos+i]); };
    const auto a = b(0);
    if (a < 0x80) return {.code_point = a, .bytes = 1};
    const int n = a >= 0xC2 && a <= 0xDF ? 2 : a >= 0xE0 && a <= 0xEF ? 3 : a >= 0xF0 && a <= 0xF4 ? 4 : 0;
    if (!n || pos + n > s.size()) return {.code_point = 0xFFFD, .bytes = 1};
    char32_t cp = a & (n == 2 ? 0x1F : n == 3 ? 0x0F : 0x07);
    for (int i = 1; i < n; ++i)
    {
        if ((b(i) & 0xC0) != 0x80) return {.code_point = 0xFFFD, .bytes = 1};
        cp = (cp << 6) | (b(i) & 0x3F);
    }
    if ((n == 2 && cp < 0x80) || (n == 3 && cp < 0x800) ||
        (n == 4 && cp < 0x10000) || (cp >= 0xD800 && cp <= 0xDFFF) || cp > 0x10FFFF)
        return {.code_point = 0xFFFD, .bytes = 1};
    return {.code_point = cp, .bytes = static_cast<std::size_t>(n)};
}

inline bool isValidUtf8(const std::string_view s)
{
    for (std::size_t i = 0; i < s.size();)
    {
        auto [code_point, bytes] = decodeUtf8(s, i);
        if (code_point == 0xFFFD && bytes == 1) return false;
        i += bytes;
    }
    return true;
}

// Codepoint width heuristic; use grapheme.h for cluster boundaries and terminal widths.
inline int codePointWidth(char32_t c)
{
    if (!c || c < 32 || (c >= 0x7F && c < 0xA0)) return 0;
    if ((c >= 0x300 && c <= 0x36F) || (c >= 0xFE00 && c <= 0xFE0F) ||
        c == 0x200D || c == 0x200C || c == 0x20E3 || (c >= 0x1F3FB && c <= 0x1F3FF)) return 0;
    const auto within = [c](const char32_t lo, const char32_t hi) { return c >= lo && c <= hi; };
    // Emoji presented as double-width glyphs in common terminals, including Terminal.app.
    // These symbols sit outside the broad supplementary emoji ranges below.
    if (c == 0x2705 || c == 0x2B1C || c == 0x2B1B || c == 0x26AA || c == 0x26AB) return 2;
    return (within(0x1100,0x115F) || within(0x2329,0x232A) ||
        within(0x2E80,0xA4CF) || within(0xAC00,0xD7A3) ||
        within(0xF900,0xFAFF) || within(0xFE10,0xFE6F) ||
        within(0xFF00,0xFF60) || within(0xFFE0,0xFFE6) ||
        within(0x1F300,0x1FAFF)) ? 2 : 1;
}

} /* End of namespace terion */

#endif /* TERION_UTF8_H */
