/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#ifndef TERION_COMPONENTS_TEXT_VIEW_DETAIL_H
#define TERION_COMPONENTS_TEXT_VIEW_DETAIL_H

#include <algorithm>
#include <iterator>
#include <utility>
#include <string>
#include <string_view>
#include <vector>
#include <terion/styled_text.h>
#include <terion/utf8.h>

namespace terion::detail {

inline int textCells(std::string_view value)
{
    int cells = 0;
    for (std::size_t pos = 0; pos < value.size();)
    {
        const auto glyph = decodeUtf8(value,pos);
        if (!glyph.bytes) break;
        cells += codePointWidth(glyph.code_point);
        pos += glyph.bytes;
    }
    return cells;
}

inline void appendSpan(StyledText& target,std::string text,const ColorVariant color,const bool bold)
{
    if (text.empty()) return;
    if (!target.empty() && target.back().color == color && target.back().bold == bold)
        target.back().text += text;
    else target.push_back({.text = std::move(text),.color = color,.bold = bold});
}

// UTF-8-aware, style-preserving wrapping. Prefer word boundaries and keep
// indentation on continuation lines. A too-wide glyph is never split.
inline std::vector<StyledText> wrapStyled(const StyledText& source,int width,int hanging = 0)
{
    if (width <= 0) return {source};
    width = std::max(1,width);
    hanging = std::clamp(hanging,0,width-1);
    std::vector<StyledText> result(1);
    int used = 0;
    bool continuation = false;
    const auto nextLine = [&] {
        result.emplace_back();
        used = 0;
        continuation = true;
        if (hanging > 0)
        {
            appendSpan(result.back(),std::string(static_cast<std::size_t>(hanging),' '),
                       ColorVariant::Default,false);
            used = hanging;
        }
    };
    // Collect word tokens across styling boundaries, retaining original spans.
    struct Glyph { std::string bytes; int cells; ColorVariant color; bool bold; bool space; };
    std::vector<Glyph> glyphs;
    for (const auto& span : source)
        for (std::size_t pos = 0; pos < span.text.size();)
        {
            const auto decoded = decodeUtf8(span.text,pos);
            if (decoded.bytes == 0) break;
            const bool space = decoded.code_point == ' ' || decoded.code_point == '\t';
            glyphs.push_back({span.text.substr(pos,decoded.bytes),
                decoded.code_point == '\t' ? 4 : codePointWidth(decoded.code_point),
                span.color,span.bold,space});
            pos += decoded.bytes;
        }
    for (std::size_t i = 0; i < glyphs.size();)
    {
        const bool whitespace = glyphs[i].space;
        std::size_t end = i;
        int token_width = 0;
        while (end < glyphs.size() && glyphs[end].space == whitespace)
        {
            token_width += glyphs[end].cells;
            ++end;
        }
        if (whitespace && continuation && used == hanging)
        {
            i = end;
            continue;
        }
        if (!whitespace && used > 0 && token_width <= width-hanging && used+token_width > width)
            nextLine();
        if (whitespace && used+token_width > width)
        {
            nextLine();
            i = end;
            continue;
        }
        for (; i < end; ++i)
        {
            const auto& glyph = glyphs[i];
            if (used > 0 && used+glyph.cells > width) nextLine();
            appendSpan(result.back(),glyph.bytes == "\t" ? "    " : glyph.bytes,glyph.color,glyph.bold);
            used += glyph.cells;
        }
    }
    return result;
}

inline std::vector<StyledText> wrapLines(const std::vector<StyledText>& lines,int width)
{
    if (width <= 0) return lines;
    std::vector<StyledText> result;
    for (const auto& line : lines)
    {
        int hanging = 0;
        if (!line.empty() && line.front().text.size() <= 12)
        {
            const auto& prefix = line.front().text;
            if (prefix == "│ " || prefix == "▌ " || prefix == "  • " ||
                prefix == "  □ " || prefix == "  ☑ " ||
                (prefix.size() >= 4 && prefix.substr(0,2) == "  " && prefix.back() == ' '))
                for (std::size_t i = 0; i < prefix.size();)
                {
                    const auto d = decodeUtf8(prefix,i);
                    hanging += codePointWidth(d.code_point);
                    i += d.bytes;
                }
        }
        auto rows = wrapStyled(line,width,hanging);
        result.insert(result.end(),std::make_move_iterator(rows.begin()),std::make_move_iterator(rows.end()));
    }
    return result;
}

} /* End of namespace terion::detail */

#endif /* TERION_COMPONENTS_TEXT_VIEW_DETAIL_H */
