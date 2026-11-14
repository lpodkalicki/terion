/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */
#ifndef TERION_COMPONENTS_TABLE_VIEW_DETAIL_H
#define TERION_COMPONENTS_TABLE_VIEW_DETAIL_H

#include <algorithm>
#include <string>
#include <string_view>
#include <utility>
#include <terion/grapheme.h>
#include <terion/styled_text.h>

namespace terion::detail {

// Slice by terminal columns, never by UTF-8 bytes. A partly visible grapheme
// becomes spaces, so wide emoji cannot leave a dangling continuation cell.
inline StyledText sliceStyledColumns(const StyledText& spans, int offset, int width)
{
    StyledText result;
    if (width <= 0) return result;
    offset = std::max(0,offset);
    const long long begin = offset;
    const long long end = begin+width;
    long long x = 0;
    for (const auto& span : spans)
    {
        auto out = span;
        out.text.clear();
        const std::string_view text = span.text;
        for (std::size_t byte = 0; byte < text.size();)
        {
            const auto g = nextGrapheme(text,byte);
            if (!g.bytes) break;
            const long long next = x+std::max(0,g.columns);
            if (next > begin && x < end)
            {
                if (x >= begin && next <= end)
                    out.text.append(text.substr(byte,g.bytes));
                else
                    out.text.append(static_cast<std::size_t>(std::min(next,end)-std::max(x,begin)),' ');
            }
            byte += g.bytes;
            x = next;
            if (x >= end) break;
        }
        if (!out.text.empty()) result.emplace_back(std::move(out));
        if (x >= end) break;
    }
    if (x < end)
        result.push_back({.text = std::string(static_cast<std::size_t>(end-std::max(x,begin)),' ')});
    return result;
}

inline std::string sliceColumns(std::string_view text, int offset, int width)
{
    StyledText spans{{.text = std::string(text)}};
    std::string result;
    for (const auto& part : sliceStyledColumns(spans,offset,width)) result += part.text;
    return result;
}

} /* End of namespace terion::detail */
#endif /* TERION_COMPONENTS_TABLE_VIEW_DETAIL_H */
