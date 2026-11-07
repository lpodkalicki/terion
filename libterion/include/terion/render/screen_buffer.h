/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#ifndef TERION_RENDER_SCREEN_BUFFER_H
#define TERION_RENDER_SCREEN_BUFFER_H

#include <vector>
#include <terion/render/cell.h>
#include <terion/geometry.h>

namespace terion {

class ScreenBuffer
{
public:
    explicit ScreenBuffer(Size size = {});
    void resize(Size size);
    void clear(const Cell& fill = {});
    [[nodiscard]] Size size() const noexcept { return size_; }
    [[nodiscard]] bool contains(int x, int y) const noexcept;
    [[nodiscard]] Cell& at(int x, int y);
    [[nodiscard]] const Cell& at(int x, int y) const;

private:
    Size size_{};
    std::vector<Cell> cells_;
}; /* End of struct ScreenBuffer */

} /* End of namespace terion */

#endif /* TERION_RENDER_SCREEN_BUFFER_H */
