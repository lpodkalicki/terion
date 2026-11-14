/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <algorithm>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <terion/render/canvas.h>

namespace terion::render {

namespace {

std::string braille(std::uint8_t bits)
{
    const auto codepoint = static_cast<std::uint32_t>(0x2800u + bits);
    return {static_cast<char>(0xe0 | (codepoint >> 12)),
            static_cast<char>(0x80 | ((codepoint >> 6) & 0x3f)),
            static_cast<char>(0x80 | (codepoint & 0x3f))};
}

} /* End of anonymous namespace */

Canvas::Canvas(int width, int height) :
    width_(width), height_(height)
{
    if (width < 0 || height < 0)
        throw std::invalid_argument("negative canvas dimensions");
    cells_.resize(static_cast<std::size_t>((width + 1) / 2) * static_cast<std::size_t>((height + 3) / 4));
}

void Canvas::clear()
{
    std::fill(cells_.begin(),cells_.end(),0);
}

void Canvas::point(int x, int y, bool enabled)
{
    if (x < 0 || y < 0 || x >= width_ || y >= height_)
        return;
    constexpr std::uint8_t dots[4][2] = {{0,3},{1,4},{2,5},{6,7}};
    const auto columns = static_cast<std::size_t>((width_ + 1) / 2);
    auto& cell = cells_[static_cast<std::size_t>(y / 4) * columns + static_cast<std::size_t>(x / 2)];
    const auto bit = static_cast<std::uint8_t>(1u << dots[y % 4][x % 2]);
    if (enabled)
        cell |= bit;
    else
        cell &= static_cast<std::uint8_t>(~bit);
}

void Canvas::line(Point start, Point end)
{
    int dx = std::abs(end.x - start.x);
    int dy = -std::abs(end.y - start.y);
    const int sx = start.x < end.x ? 1 : -1;
    const int sy = start.y < end.y ? 1 : -1;
    int error = dx + dy;
    while (true)
    {
        point(start.x,start.y);
        if (start.x == end.x && start.y == end.y)
            break;
        const int twice = 2 * error;
        if (twice >= dy)
        {
            error += dy;
            start.x += sx;
        }
        if (twice <= dx)
        {
            error += dx;
            start.y += sy;
        }
    }
}

void Canvas::rectangle(Rect rect)
{
    if (rect.width <= 0 || rect.height <= 0)
        return;
    line({rect.x,rect.y},{rect.x + rect.width - 1,rect.y});
    line({rect.x,rect.y + rect.height - 1},{rect.x + rect.width - 1,rect.y + rect.height - 1});
    line({rect.x,rect.y},{rect.x,rect.y + rect.height - 1});
    line({rect.x + rect.width - 1,rect.y},{rect.x + rect.width - 1,rect.y + rect.height - 1});
}

void Canvas::circle(Point center, int radius)
{
    if (radius < 0)
        return;
    int x = radius;
    int y = 0;
    int error = 1 - radius;
    while (x >= y)
    {
        point(center.x + x,center.y + y);
        point(center.x + y,center.y + x);
        point(center.x - y,center.y + x);
        point(center.x - x,center.y + y);
        point(center.x - x,center.y - y);
        point(center.x - y,center.y - x);
        point(center.x + y,center.y - x);
        point(center.x + x,center.y - y);
        ++y;
        if (error < 0)
            error += 2 * y + 1;
        else
        {
            --x;
            error += 2 * (y - x) + 1;
        }
    }
}

std::vector<std::string> Canvas::rows() const
{
    const int columns = (width_+1)/2;
    const int rows = (height_+3)/4;
    std::vector<std::string> result;
    result.reserve(static_cast<std::size_t>(rows));
    for (int y = 0; y < rows; ++y)
    {
        std::string row;
        for (int x = 0; x < columns; ++x)
            row += braille(cells_[static_cast<std::size_t>(y*columns+x)]);
        result.push_back(std::move(row));
    }
    return result;
}

void Canvas::paint(Painter& painter, Point origin, TextStyle style) const
{
    const auto columns = (width_ + 1) / 2;
    const auto rows = (height_ + 3) / 4;
    for (int y = 0; y < rows; ++y)
        for (int x = 0; x < columns; ++x)
            painter.put(origin.x + x,origin.y + y,
                        braille(cells_[static_cast<std::size_t>(y * columns + x)]),style);
}

} /* End of namespace terion::render */
