/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#ifndef TERION_TEXT_EDITOR_CORE_H
#define TERION_TEXT_EDITOR_CORE_H

#include <algorithm>
#include <string>
#include <string_view>
#include <terion/utf8.h>

namespace terion::detail {

inline std::size_t nextCodepoint(std::string_view text, std::size_t cursor)
{
    if (cursor >= text.size()) return text.size();
    return std::min(text.size(),cursor+std::max<std::size_t>(1,decodeUtf8(text,cursor).bytes));
}

inline std::size_t previousCodepoint(std::string_view text, std::size_t cursor)
{
    std::size_t previous = 0;
    for (std::size_t i = 0; i < cursor;)
    {
        const auto decoded = decodeUtf8(text,i);
        if (!decoded.bytes || i + decoded.bytes >= cursor) return i;
        previous = i;
        i += decoded.bytes;
    }
    return previous;
}

inline std::size_t previousBoundary(std::string_view text, std::size_t cursor)
{
    std::size_t start = previousCodepoint(text,cursor);
    while (start > 0 && codePointWidth(decodeUtf8(text,start).code_point) == 0)
        start = previousCodepoint(text,start);
    return start;
}

inline std::size_t nextBoundary(std::string_view text, std::size_t cursor)
{
    std::size_t end = nextCodepoint(text,cursor);
    while (end < text.size() && codePointWidth(decodeUtf8(text,end).code_point) == 0)
        end = nextCodepoint(text,end);
    return end;
}

inline std::string encodeUtf8(char32_t code_point)
{
    const char32_t c = code_point > 0x10FFFF || (code_point >= 0xD800 && code_point <= 0xDFFF) ? 0xFFFD : code_point;
    std::string result;
    if (c < 0x80) result += static_cast<char>(c);
    else if (c < 0x800)
    {
        result += static_cast<char>(0xC0 | (c >> 6));
        result += static_cast<char>(0x80 | (c & 0x3F));
    }
    else if (c < 0x10000)
    {
        result += static_cast<char>(0xE0 | (c >> 12));
        result += static_cast<char>(0x80 | ((c >> 6) & 0x3F));
        result += static_cast<char>(0x80 | (c & 0x3F));
    }
    else
    {
        result += static_cast<char>(0xF0 | (c >> 18));
        result += static_cast<char>(0x80 | ((c >> 12) & 0x3F));
        result += static_cast<char>(0x80 | ((c >> 6) & 0x3F));
        result += static_cast<char>(0x80 | (c & 0x3F));
    }
    return result;
}

inline std::string sanitizePaste(std::string_view text, bool multiline)
{
    std::string result;
    for (std::size_t i = 0; i < text.size();)
    {
        const auto decoded = decodeUtf8(text,i);
        if (!decoded.bytes) break;
        const auto c = decoded.code_point;
        if ((c >= 32 && c != 127 && c != 0xFFFD) || (multiline && (c == 10 || c == 9)))
            result.append(text.substr(i,decoded.bytes));
        i += decoded.bytes;
    }
    return result;
}

} /* End of namespace terion::detail */

#endif /* TERION_TEXT_EDITOR_CORE_H */
