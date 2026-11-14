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
#include <terion/grapheme.h>

namespace terion::detail {

inline int textCells(std::string_view value)
{
    return displayWidth(value);
}

inline void appendSpan(StyledText& target,StyledSpan span)
{
    if (span.text.empty()) return;
    if (!target.empty() && target.back().color == span.color &&
        target.back().bold == span.bold && target.back().italic == span.italic &&
        target.back().underline == span.underline &&
        target.back().strikethrough == span.strikethrough &&
        target.back().dim == span.dim && target.back().background == span.background)
        target.back().text += span.text;
    else target.push_back(std::move(span));
}

inline void appendSpan(StyledText& target,std::string text,const ColorVariant color,const bool bold)
{
    appendSpan(target,{.text = std::move(text),.color = color,.bold = bold});
}

// Word-wrap never splits an extended grapheme cluster or drops its styling.
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
    struct Glyph { StyledSpan span; int cells; bool space; };
    std::vector<Glyph> glyphs;
    for (const auto& span : source)
        for (std::size_t pos = 0; pos < span.text.size();)
        {
            const auto cluster = nextGrapheme(span.text,pos);
            if (!cluster.bytes) break;
            const auto bytes = span.text.substr(pos,cluster.bytes);
            const bool space = bytes == " " || bytes == "\t";
            StyledSpan styled = span;
            styled.text = bytes == "\t" ? "    " : bytes;
            glyphs.push_back({std::move(styled),bytes == "\t" ? 4 : cluster.columns,space});
            pos += cluster.bytes;
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
            appendSpan(result.back(),glyph.span);
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
                    const auto cluster = nextGrapheme(prefix,i);
                    if (!cluster.bytes) break;
                    hanging += cluster.columns;
                    i += cluster.bytes;
                }
        }
        auto rows = wrapStyled(line,width,hanging);
        result.insert(result.end(),std::make_move_iterator(rows.begin()),std::make_move_iterator(rows.end()));
    }
    return result;
}

} /* End of namespace terion::detail */

#endif /* TERION_COMPONENTS_TEXT_VIEW_DETAIL_H */
