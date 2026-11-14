/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <algorithm>
#include <terion/grapheme.h>
#include <terion/render/painter.h>

using namespace terion;

void Painter::pushClip(const Rect rect)
{
    clip_stack_.push_back(clip_);
    if (!clip_)
    {
        clip_ = rect;
        return;
    }
    const int x = std::max(clip_->x,rect.x);
    const int y = std::max(clip_->y,rect.y);
    const long long right = std::min(static_cast<long long>(clip_->x) + clip_->width,
        static_cast<long long>(rect.x) + rect.width);
    const long long bottom = std::min(static_cast<long long>(clip_->y) + clip_->height,
        static_cast<long long>(rect.y) + rect.height);
    clip_ = {.x = x,.y = y,.width = static_cast<int>(std::max(0LL,right-x)),
        .height = static_cast<int>(std::max(0LL,bottom-y))};
}

void Painter::popClip()
{
    if (clip_stack_.empty()) return;
    clip_ = clip_stack_.back();
    clip_stack_.pop_back();
}

bool Painter::visible(const int x, const int y) const noexcept
{
    return buffer_.contains(x,y) && (!clip_ ||
        (x >= clip_->x && y >= clip_->y && static_cast<long long>(x) < static_cast<long long>(clip_->x) + clip_->width &&
         static_cast<long long>(y) < static_cast<long long>(clip_->y) + clip_->height));
}

void Painter::eraseGlyphAt(const int x, const int y) const
{
    if (!visible(x,y)) return;
    auto& cell = buffer_.at(x,y);
    if (cell.width == 0 && visible(x-1,y)) buffer_.at(x-1,y) = {};
    if (cell.width == 2 && visible(x+1,y)) buffer_.at(x+1,y) = {};
    cell = {};
}

void Painter::put(const int x, const int y, const std::string_view grapheme, const TextStyle style) const
{
    if (grapheme.empty() || !visible(x,y)) return;
    const int width = graphemeWidth(grapheme);
    if (width < 1 || (width == 2 && !visible(x+1,y))) return;
    eraseGlyphAt(x,y);
    if (width == 2) eraseGlyphAt(x+1,y);
    buffer_.at(x,y) = {.grapheme = std::string(grapheme),.style = style,.width = static_cast<unsigned char>(width)};
    if (width == 2) buffer_.at(x+1,y) = {.grapheme = "",.style = style,.width = 0};
}

void Painter::text(int x, const int y, const std::string_view text, const TextStyle style) const
{
    for (std::size_t i = 0; i < text.size();)
    {
        const auto grapheme = nextGrapheme(text,i);
        if (!grapheme.bytes) break;
        if (grapheme.columns > 0)
        {
            if (visible(x,y) && (grapheme.columns == 1 || visible(x+1,y)))
                put(x,y,text.substr(i,grapheme.bytes),style);
            x += grapheme.columns;
        }
        i += grapheme.bytes;
    }
}

void Painter::fill(const Rect rect, Cell cell) const
{
    const int x0 = std::max({0,rect.x,clip_ ? clip_->x : 0});
    const int y0 = std::max({0,rect.y,clip_ ? clip_->y : 0});
    const int x1 = std::min({buffer_.size().width,rect.x + rect.width,
        clip_ ? clip_->x + clip_->width : buffer_.size().width});
    const int y1 = std::min({buffer_.size().height,rect.y + rect.height,
        clip_ ? clip_->y + clip_->height : buffer_.size().height});
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
    const int x0 = std::max({0,rect.x,clip_ ? clip_->x : 0});
    const int y0 = std::max({0,rect.y,clip_ ? clip_->y : 0});
    const int x1 = std::min({buffer_.size().width,rect.x + rect.width,
        clip_ ? clip_->x + clip_->width : buffer_.size().width});
    const int y1 = std::min({buffer_.size().height,rect.y + rect.height,
        clip_ ? clip_->y + clip_->height : buffer_.size().height});
    for (int y = y0; y < y1; ++y)
        for (int x = x0; x < x1; ++x)
        {
            buffer_.at(x,y).style.background = color;
            if (buffer_.at(x,y).width == 0 && visible(x-1,y))
                buffer_.at(x-1,y).style.background = color;
        }
}

void Painter::dim(const Rect rect, const Color background, const int percent) const
{
    const int opacity = std::clamp(percent,0,100);
    const auto blend = [&](const Color color) {
        const auto channel = [&](const std::uint8_t a, const std::uint8_t b) {
            return static_cast<std::uint8_t>((static_cast<int>(a)*(100-opacity)+
                static_cast<int>(b)*opacity)/100);
        };
        return Color::rgb(channel(color.red,background.red),channel(color.green,background.green),
            channel(color.blue,background.blue));
    };
    const int left = std::max(0,rect.x);
    const int top = std::max(0,rect.y);
    const int right = std::min(buffer_.size().width,rect.x+rect.width);
    const int bottom = std::min(buffer_.size().height,rect.y+rect.height);
    for (int y = top; y < bottom; ++y)
        for (int x = left; x < right; ++x)
        {
            if (!visible(x,y)) continue;
            auto& style = buffer_.at(x,y).style;
            style.foreground = blend(style.foreground);
            style.background = blend(style.background);
        }
}

void Painter::styleAt(int x, const int y, const TextStyle style) const
{
    if (!visible(x,y)) return;
    if (buffer_.at(x,y).width == 0 && visible(x-1,y)) --x;
    auto& cell = buffer_.at(x,y);
    cell.style = style;
    if (cell.width == 2 && visible(x+1,y)) buffer_.at(x+1,y).style = style;
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
