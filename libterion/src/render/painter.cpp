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
    int last_x = -1;
    for (std::size_t i = 0; i < text.size();)
    {
        const auto [code_point, bytes] = decodeUtf8(text,i);
        if (bytes == 0) break;
        const int width = codePointWidth(code_point);
        if (width == 0 && last_x >= 0)
        {
            buffer_.at(last_x,y).grapheme.append(text.substr(i,bytes));
        }
        else if (width > 0)
        {
            if (buffer_.contains(x,y) && (width == 1 || buffer_.contains(x+1,y)))
            {
                put(x,y,text.substr(i,bytes),style);
                last_x = x;
            }
            else last_x = -1;
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

void Painter::fillBackground(const Rect rect, const Color color) const
{
    const int x0 = std::max(0,rect.x), y0 = std::max(0,rect.y);
    const int x1 = std::min(buffer_.size().width,rect.x + rect.width);
    const int y1 = std::min(buffer_.size().height,rect.y + rect.height);
    for (int y = y0; y < y1; ++y)
        for (int x = x0; x < x1; ++x)
        {
            buffer_.at(x,y).style.background = color;
            if (buffer_.at(x,y).width == 0 && buffer_.contains(x-1,y))
                buffer_.at(x-1,y).style.background = color;
        }
}

void Painter::horizontalLine(const int x, const int y, const int length, const TextStyle style) const
{
    for (int i = 0; i < length; ++i) put(x+i,y,"─",style);
}

void Painter::verticalLine(const int x, const int y, const int length, const TextStyle style) const
{
    for (int i = 0; i < length; ++i) put(x,y+i,"│",style);
}

void Painter::line(const Point start, const Point end, const TextStyle style) const
{
    int x = start.x, y = start.y;
    const int dx = std::abs(end.x - start.x), dy = std::abs(end.y - start.y);
    const int sx = start.x < end.x ? 1 : -1, sy = start.y < end.y ? 1 : -1;
    int error = dx - dy;
    while (true)
    {
        put(x,y,dx == 0 ? "│" : dy == 0 ? "─" : sx == sy ? "╲" : "╱",style);
        if (x == end.x && y == end.y) break;
        const int twice = error * 2;
        if (twice > -dy) { error -= dy; x += sx; }
        if (twice < dx) { error += dx; y += sy; }
    }
}

void Painter::border(const Rect rect, const BorderStyle border_style, const TextStyle style) const
{
    if (rect.width <= 0 || rect.height <= 0) return;
    const char* horizontal = border_style == BorderStyle::Double ? "═" : border_style == BorderStyle::Heavy ? "━" : "─";
    const char* vertical = border_style == BorderStyle::Double ? "║" : border_style == BorderStyle::Heavy ? "┃" : "│";
    const char* tl = border_style == BorderStyle::Double ? "╔" : border_style == BorderStyle::Rounded ? "╭" : border_style == BorderStyle::Heavy ? "┏" : "┌";
    const char* tr = border_style == BorderStyle::Double ? "╗" : border_style == BorderStyle::Rounded ? "╮" : border_style == BorderStyle::Heavy ? "┓" : "┐";
    const char* bl = border_style == BorderStyle::Double ? "╚" : border_style == BorderStyle::Rounded ? "╰" : border_style == BorderStyle::Heavy ? "┗" : "└";
    const char* br = border_style == BorderStyle::Double ? "╝" : border_style == BorderStyle::Rounded ? "╯" : border_style == BorderStyle::Heavy ? "┛" : "┘";
    for (int x = rect.x+1; x < rect.x+rect.width-1; ++x)
    {
        put(x,rect.y,horizontal,style);
        if (rect.height > 1) put(x,rect.y+rect.height-1,horizontal,style);
    }
    for (int y = rect.y+1; y < rect.y+rect.height-1; ++y)
    {
        put(rect.x,y,vertical,style);
        if (rect.width > 1) put(rect.x+rect.width-1,y,vertical,style);
    }
    put(rect.x,rect.y,tl,style);
    if (rect.width > 1) put(rect.x+rect.width-1,rect.y,tr,style);
    if (rect.height > 1) put(rect.x,rect.y+rect.height-1,bl,style);
    if (rect.width > 1 && rect.height > 1) put(rect.x+rect.width-1,rect.y+rect.height-1,br,style);
}
