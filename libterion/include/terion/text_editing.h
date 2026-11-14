/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#ifndef TERION_TEXT_EDITING_H
#define TERION_TEXT_EDITING_H

#include <terion/text_editor_core.h>
#include <algorithm>
#include <cstddef>
#include <string_view>
#include <utility>
#include <vector>

namespace terion::detail {

enum class WordClass { Space, Word, Symbol };

inline WordClass wordClass(char32_t codepoint) noexcept
{
    if (codepoint == U' ' || codepoint == U'\t' || codepoint == U'\r' || codepoint == U'\n')
        return WordClass::Space;
    if ((codepoint >= U'a' && codepoint <= U'z') || (codepoint >= U'A' && codepoint <= U'Z') ||
        (codepoint >= U'0' && codepoint <= U'9') || codepoint == U'_' || codepoint >= 0x80)
        return WordClass::Word;
    return WordClass::Symbol;
}

inline WordClass wordClassAt(std::string_view text, std::size_t byte)
{
    return wordClass(decodeUtf8(text,byte).code_point);
}

// Keep all offsets on UTF-8 character boundaries. Non-ASCII letters are words;
// punctuation and whitespace are separate runs (not locale-dependent).
inline std::size_t nextWordBoundary(std::string_view text, std::size_t byte)
{
    byte = std::min(byte,text.size());
    if (byte == text.size()) return byte;
    if (wordClassAt(text,byte) != WordClass::Word)
        while (byte < text.size() && wordClassAt(text,byte) != WordClass::Word)
            byte = nextBoundary(text,byte);
    while (byte < text.size() && wordClassAt(text,byte) == WordClass::Word)
        byte = nextBoundary(text,byte);
    return byte;
}

inline std::size_t previousWordBoundary(std::string_view text, std::size_t byte)
{
    byte = std::min(byte,text.size());
    while (byte > 0)
    {
        const auto previous = previousBoundary(text,byte);
        if (wordClassAt(text,previous) == WordClass::Word) break;
        byte = previous;
    }
    while (byte > 0)
    {
        const auto previous = previousBoundary(text,byte);
        if (wordClassAt(text,previous) != WordClass::Word) break;
        byte = previous;
    }
    return byte;
}

inline std::pair<std::size_t,std::size_t> wordAt(std::string_view text, std::size_t byte)
{
    if (text.empty()) return {0,0};
    byte = std::min(byte,text.size());
    if (byte == text.size()) byte = previousBoundary(text,byte);
    const auto kind = wordClassAt(text,byte);
    std::size_t begin = byte;
    std::size_t end = byte;
    while (begin > 0)
    {
        const auto previous = previousBoundary(text,begin);
        if (wordClassAt(text,previous) != kind) break;
        begin = previous;
    }
    while (end < text.size() && wordClassAt(text,end) == kind)
        end = nextBoundary(text,end);
    return {begin,end};
}

struct VisualTextLine
{
    std::size_t begin = 0;
    std::size_t end = 0;
};

// Character-wrap at terminal cell boundaries. A newline consumes one source
// byte but never becomes part of a visual row. Empty trailing lines are kept.
inline std::vector<VisualTextLine> visualTextLines(std::string_view text, int width, bool wrap)
{
    std::vector<VisualTextLine> result;
    width = std::max(1,width);
    std::size_t start = 0;
    int columns = 0;
    for (std::size_t i = 0; i < text.size();)
    {
        if (text[i] == '\n')
        {
            result.push_back({start,i});
            start = ++i;
            columns = 0;
            continue;
        }
        const auto decoded = decodeUtf8(text,i);
        const std::size_t bytes = std::max<std::size_t>(1,decoded.bytes);
        const int cell_width = std::max(0,codePointWidth(decoded.code_point));
        if (wrap && columns + cell_width > width && i > start)
        {
            result.push_back({start,i});
            start = i;
            columns = 0;
        }
        columns += cell_width;
        i += bytes;
    }
    result.push_back({start,text.size()});
    // An insertion caret after a line that exactly fills the viewport needs
    // its own visual row; the last character must remain visible.
    if (wrap && columns >= width && start < text.size() && text.back() != '\n')
        result.push_back({text.size(),text.size()});
    return result;
}

inline std::size_t visualLineIndex(const std::vector<VisualTextLine>& lines, std::size_t byte)
{
    for (std::size_t i = 0; i+1 < lines.size(); ++i)
        if (byte < lines[i].end || (byte == lines[i].end && lines[i+1].begin != byte))
            return i;
    return lines.size()-1;
}

inline int visualColumn(std::string_view text, VisualTextLine line, std::size_t byte)
{
    int column = 0;
    for (std::size_t i = line.begin; i < std::min(line.end,byte);)
    {
        const auto decoded = decodeUtf8(text,i);
        if (!decoded.bytes) break;
        column += std::max(0,codePointWidth(decoded.code_point));
        i += decoded.bytes;
    }
    return column;
}

inline std::size_t visualOffset(std::string_view text, VisualTextLine line, int column)
{
    int width = 0;
    std::size_t byte = line.begin;
    while (byte < line.end)
    {
        const auto decoded = decodeUtf8(text,byte);
        if (!decoded.bytes || width + codePointWidth(decoded.code_point) > column) break;
        width += std::max(0,codePointWidth(decoded.code_point));
        byte += decoded.bytes;
    }
    return byte;
}

} /* End of namespace terion::detail */

#endif /* TERION_TEXT_EDITING_H */
