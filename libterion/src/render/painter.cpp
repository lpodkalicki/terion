/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <algorithm>
#include <terion/utf8.h>
#include <terion/render/painter.h>

using namespace terion;

void Painter::eraseGlyphAt(const int x, const int y) const
{
    if (!buffer_.contains(x,y)) return;
    auto& cell = buffer_.at(x,y);
    if (cell.width == 0 && buffer_.contains(x-1,y)) buffer_.at(x-1,y) = {};
    if (cell.width == 2 && buffer_.contains(x+1,y)) buffer_.at(x+1,y) = {};
    cell = {};
}

void Painter::put(const int x, const int y, const std::string_view grapheme, const TextStyle style) const
{
    if (grapheme.empty() || !buffer_.contains(x,y)) return;
    const auto [code_point, bytes] = decodeUtf8(grapheme,0);
    const int width = codePointWidth(code_point);
    if (width < 1 || (width == 2 && !buffer_.contains(x+1,y))) return;
    eraseGlyphAt(x,y);
    if (width == 2) eraseGlyphAt(x+1,y);
    buffer_.at(x,y) = {.grapheme = std::string(grapheme),.style = style,.width = static_cast<unsigned char>(width)};
    if (width == 2) buffer_.at(x+1,y) = {.grapheme = "",.style = style,.width = 0};
}

void Painter::text(int x, const int y, const std::string_view text, const TextStyle style) const
{
    for (std::size_t i = 0; i < text.size();) {
        auto [code_point, bytes] = decodeUtf8(text,i);
        if (bytes == 0) break;
        auto width = codePointWidth(code_point);
        if (width == 0 && i > 0 && buffer_.contains(x-1,y))
        {
            auto& prior = buffer_.at(x-1,y);
            if (prior.width == 0 && buffer_.contains(x-2,y)) buffer_.at(x-2,y).grapheme.append(text.substr(i,bytes));
            else prior.grapheme.append(text.substr(i,bytes));
        }
        else if (width > 0)
        {
            put(x,y,text.substr(i,bytes),style);
            x += width;
        }
        i += bytes;
    }
}

void Painter::fill(const Rect rect, Cell cell) const
{
    const int x0 = std::max(0,rect.x), y0 = std::max(0,rect.y);
    const int x1 = std::min(buffer_.size().width,rect.x + rect.width);
    const int y1 = std::min(buffer_.size().height,rect.y + rect.height);
    if (cell.width != 1) cell = {};
    for (int y = y0; y < y1; ++y)
    {
        for (int x = x0; x < x1; ++x)
        {
            eraseGlyphAt(x,y); buffer_.at(x,y) = cell;
        }
    }
}
