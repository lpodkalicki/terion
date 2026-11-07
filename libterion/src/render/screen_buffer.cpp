/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <stdexcept>
#include <algorithm>
#include <terion/render/screen_buffer.h>

using namespace terion;

ScreenBuffer::ScreenBuffer(const Size size)
{
    resize(size);
}

void ScreenBuffer::resize(const Size size)
{
    if (size.width < 0 || size.height < 0) throw std::invalid_argument("negative buffer size");
    size_ = size;
    cells_.assign(static_cast<std::size_t>(size.width) * static_cast<std::size_t>(size.height), Cell{});
}

void ScreenBuffer::clear(const Cell& fill)
{
    std::ranges::fill(cells_, fill);
}

bool ScreenBuffer::contains(const int x, const int y) const noexcept
{
    return x >= 0 && y >= 0 && x < size_.width && y < size_.height;
}

Cell& ScreenBuffer::at(const int x, const int y)
{
    if (!contains(x,y)) throw std::out_of_range("ScreenBuffer::at");
    return cells_[static_cast<std::size_t>(y) * size_.width + x];
}

const Cell& ScreenBuffer::at(const int x, const int y) const
{
    if (!contains(x,y)) throw std::out_of_range("ScreenBuffer::at");
    return cells_[static_cast<std::size_t>(y) * size_.width + x];
}
