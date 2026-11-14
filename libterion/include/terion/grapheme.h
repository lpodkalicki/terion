/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#ifndef TERION_GRAPHEME_H
#define TERION_GRAPHEME_H

#include <algorithm>
#include <cstddef>
#include <string_view>
#include <terion/utf8.h>

namespace terion {

struct Grapheme
{
    std::size_t bytes = 0;
    int columns = 0;
}; /* End of struct Grapheme */

namespace unicode_detail {

inline bool in(char32_t c,char32_t lo,char32_t hi) { return c >= lo && c <= hi; }

inline bool control(char32_t c)
{
    return c == U'\r' || c == U'\n' || c < 0x20 || in(c,0x7F,0x9F) ||
        c == 0x2028 || c == 0x2029;
}

inline bool extend(char32_t c)
{
    return in(c,0x0300,0x036F) || in(c,0x0483,0x0489) || in(c,0x0591,0x05BD) ||
        in(c,0x05BF,0x05BF) || in(c,0x05C1,0x05C2) || in(c,0x0610,0x061A) ||
        in(c,0x064B,0x065F) || in(c,0x0670,0x0670) || in(c,0x06D6,0x06ED) ||
        in(c,0x0711,0x0711) || in(c,0x0730,0x074A) || in(c,0x07A6,0x07B0) ||
        in(c,0x0816,0x082D) || in(c,0x0859,0x085B) || in(c,0x08D3,0x0902) ||
        in(c,0x093A,0x093C) || in(c,0x0941,0x0948) || in(c,0x094D,0x094D) ||
        in(c,0x0951,0x0957) || in(c,0x0962,0x0963) || in(c,0x09BC,0x09BC) ||
        in(c,0x09C1,0x09C4) || in(c,0x09CD,0x09CD) || in(c,0x0A3C,0x0A3C) ||
        in(c,0x0A41,0x0A42) || in(c,0x0A4D,0x0A4D) || in(c,0x0ABC,0x0ABC) ||
        in(c,0x0AC1,0x0AC8) || in(c,0x0ACD,0x0ACD) || in(c,0x0B3C,0x0B3C) ||
        in(c,0x0B4D,0x0B4D) || in(c,0x0C3E,0x0C40) || in(c,0x0C46,0x0C56) ||
        in(c,0x0CBC,0x0CBC) || in(c,0x0CCD,0x0CCD) || in(c,0x0D3B,0x0D3C) ||
        in(c,0x0D4D,0x0D4D) || in(c,0x0E31,0x0E31) || in(c,0x0E34,0x0E3A) ||
        in(c,0x0E47,0x0E4E) || in(c,0x0EB1,0x0EB1) || in(c,0x0EB4,0x0EBC) ||
        in(c,0x0EC8,0x0ECD) || in(c,0x0F71,0x0F84) || in(c,0x102D,0x1030) ||
        in(c,0x1032,0x1037) || in(c,0x1039,0x103A) || in(c,0x135D,0x135F) ||
        in(c,0x1712,0x1714) || in(c,0x1732,0x1734) || in(c,0x1752,0x1753) ||
        in(c,0x1772,0x1773) || in(c,0x17B4,0x17B5) || in(c,0x17B7,0x17BD) ||
        in(c,0x17C6,0x17C6) || in(c,0x17C9,0x17D3) || in(c,0x180B,0x180F) ||
        in(c,0x1AB0,0x1AFF) || in(c,0x1DC0,0x1DFF) || in(c,0x20D0,0x20FF) ||
        in(c,0x302A,0x302F) || in(c,0x3099,0x309A) || in(c,0xFE00,0xFE0F) ||
        in(c,0xFE20,0xFE2F) || in(c,0x1F3FB,0x1F3FF) || in(c,0xE0100,0xE01EF) ||
        in(c,0xE0020,0xE007F) || c == 0x200C || c == 0x20E3;
}

inline bool spacingMark(char32_t c)
{
    return in(c,0x0903,0x0903) || in(c,0x093B,0x093B) || in(c,0x093E,0x0940) ||
        in(c,0x0949,0x094C) || in(c,0x0982,0x0983) || in(c,0x09BE,0x09C0) ||
        in(c,0x09C7,0x09C8) || in(c,0x09CB,0x09CC) || in(c,0x0A3E,0x0A40) ||
        in(c,0x0ABE,0x0AC0) || in(c,0x0AC9,0x0ACC) || in(c,0x0B3E,0x0B40) ||
        in(c,0x0B47,0x0B48) || in(c,0x0B4B,0x0B4C) || in(c,0x0BBE,0x0BC2) ||
        in(c,0x0BC6,0x0BC8) || in(c,0x0BCA,0x0BCC) || in(c,0x0C01,0x0C03) ||
        in(c,0x0C41,0x0C44) || in(c,0x0C82,0x0C83) || in(c,0x0CC0,0x0CC4) ||
        in(c,0x0D02,0x0D03) || in(c,0x0D3E,0x0D40) || in(c,0x0D46,0x0D48) ||
        in(c,0x0D4A,0x0D4C) || in(c,0x102B,0x102C) || in(c,0x1031,0x1031) ||
        in(c,0x1038,0x1038) || in(c,0x17B6,0x17B6) || in(c,0x17BE,0x17C5);
}

inline bool prepend(char32_t c)
{
    return in(c,0x0600,0x0605) || c == 0x06DD || c == 0x070F || c == 0x0890 ||
        c == 0x0891 || c == 0x08E2 || c == 0x110BD;
}

inline bool pictographic(char32_t c)
{
    return in(c,0x1F000,0x1FAFF) || in(c,0x2600,0x27BF) ||
        c == 0x00A9 || c == 0x00AE || c == 0x203C || c == 0x2049 ||
        c == 0x2122 || c == 0x2139 || c == 0x3030 || c == 0x303D ||
        c == 0x3297 || c == 0x3299;
}

inline bool regional(char32_t c) { return in(c,0x1F1E6,0x1F1FF); }

inline bool emojiPresentation(char32_t c)
{
    return in(c,0x231A,0x231B) || in(c,0x23E9,0x23EC) || c == 0x23F0 || c == 0x23F3 ||
        c == 0x2614 || c == 0x2615 || in(c,0x2648,0x2653) || c == 0x267F ||
        c == 0x2693 || c == 0x26A1 || c == 0x26AA || c == 0x26AB ||
        c == 0x26BD || c == 0x26BE || in(c,0x26C4,0x26C5) || c == 0x26CE ||
        c == 0x26D4 || c == 0x26EA || in(c,0x26F2,0x26F3) || c == 0x26F5 ||
        c == 0x26FA || c == 0x26FD || c == 0x2705 || in(c,0x270A,0x270B) ||
        c == 0x2728 || c == 0x274C || c == 0x274E || in(c,0x2753,0x2755) ||
        c == 0x2757 || in(c,0x2795,0x2797) || c == 0x27B0 || c == 0x27BF ||
        in(c,0x2B1B,0x2B1C) || c == 0x2B50 || c == 0x2B55;
}

inline int hangul(char32_t c)
{
    if (in(c,0x1100,0x115F) || in(c,0xA960,0xA97C)) return 1; // L
    if (in(c,0x1160,0x11A7) || in(c,0xD7B0,0xD7C6)) return 2; // V
    if (in(c,0x11A8,0x11FF) || in(c,0xD7CB,0xD7FB)) return 3; // T
    if (in(c,0xAC00,0xD7A3)) return ((c-0xAC00)%28 == 0) ? 4 : 5; // LV/LVT
    return 0;
}

} /* End of namespace unicode_detail */

// Extended grapheme clusters (UAX #29 core rules), without a Unicode database
// dependency. Property tables cover common scripts/emoji; they are not an
// exhaustive, versioned Unicode data set. Unknown codepoints remain atomic.
inline Grapheme nextGrapheme(std::string_view text,std::size_t pos)
{
    if (pos >= text.size()) return {};
    const auto first = decodeUtf8(text,pos);
    if (!first.bytes) return {};
    std::size_t end = pos + first.bytes;
    char32_t previous = first.code_point;
    int width = unicode_detail::emojiPresentation(previous) ? 2 : codePointWidth(previous);
    bool emoji = unicode_detail::pictographic(previous);
    bool has_emoji = emoji;
    bool keycap = false;
    bool variation_emoji = false;
    unsigned regional_count = unicode_detail::regional(previous) ? 1U : 0U;
    bool emoji_before_zwj = false;

    while (end < text.size())
    {
        const auto next = decodeUtf8(text,end);
        if (!next.bytes) break;
        const auto c = next.code_point;
        const int a = unicode_detail::hangul(previous);
        const int b = unicode_detail::hangul(c);
        bool join = previous == U'\r' && c == U'\n';
        if (!join && (unicode_detail::control(previous) || unicode_detail::control(c))) break;
        if (!join)
            join = (a == 1 && (b == 1 || b == 2 || b == 4 || b == 5)) ||
                ((a == 2 || a == 4) && (b == 2 || b == 3)) ||
                ((a == 3 || a == 5) && b == 3) ||
                unicode_detail::extend(c) || unicode_detail::spacingMark(c) ||
                c == 0x200D || unicode_detail::prepend(previous) ||
                (previous == 0x200D && emoji_before_zwj && unicode_detail::pictographic(c)) ||
                (unicode_detail::regional(previous) && unicode_detail::regional(c) && regional_count%2 == 1);
        if (!join) break;
        if (c == 0xFE0F && has_emoji) variation_emoji = true;
        if (c == 0x20E3) keycap = true;
        if (c == 0x200D) emoji_before_zwj = has_emoji;
        if (unicode_detail::pictographic(c)) has_emoji = true;
        if (c != 0x200D && !unicode_detail::extend(c)) emoji_before_zwj = false;
        if (unicode_detail::regional(c)) ++regional_count;
        else if (!unicode_detail::extend(c) && c != 0x200D) regional_count = 0;
        if (!unicode_detail::extend(c) && c != 0x200D && !unicode_detail::spacingMark(c))
            width = std::max(width,unicode_detail::emojiPresentation(c) ? 2 : codePointWidth(c));
        previous = c;
        end += next.bytes;
    }
    if (regional_count >= 2 || keycap || variation_emoji || (has_emoji && end > pos+first.bytes && width == 2))
        width = std::max(width,2);
    // Standalone combining marks have no base; allow rendering in one cell.
    return {.bytes = end-pos,.columns = std::clamp(width,0,2)};
}

inline int graphemeWidth(std::string_view grapheme)
{
    return nextGrapheme(grapheme,0).columns;
}

inline int displayWidth(std::string_view text)
{
    int width = 0;
    for (std::size_t pos = 0; pos < text.size();)
    {
        const auto grapheme = nextGrapheme(text,pos);
        if (!grapheme.bytes) break;
        width += grapheme.columns;
        pos += grapheme.bytes;
    }
    return width;
}

inline std::size_t nextGraphemeBoundary(std::string_view text,std::size_t pos)
{
    pos = std::min(pos,text.size());
    return std::min(text.size(),pos+nextGrapheme(text,pos).bytes);
}

inline std::size_t previousGraphemeBoundary(std::string_view text,std::size_t pos)
{
    pos = std::min(pos,text.size());
    std::size_t previous = 0;
    for (std::size_t i = 0; i < pos;)
    {
        previous = i;
        const auto next = nextGraphemeBoundary(text,i);
        if (next <= i || next >= pos) return i;
        i = next;
    }
    return previous;
}

} /* End of namespace terion */

#endif /* TERION_GRAPHEME_H */
