/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#ifndef TERION_RENDER_CANVAS_H
#define TERION_RENDER_CANVAS_H

#include <cstdint>
#include <string>
#include <vector>
#include <terion/render/painter.h>

namespace terion::render {

class Canvas
{
public:
    Canvas(int width, int height);

    int width() const { return width_; }
    int height() const { return height_; }
    void clear();
    void point(int x, int y, bool enabled = true);
    void line(Point start, Point end);
    void rectangle(Rect rect);
    void circle(Point center, int radius);
    void paint(Painter& painter, Point origin = {}, TextStyle style = {}) const;
    [[nodiscard]] std::vector<std::string> rows() const;

private:
    int width_;
    int height_;
    std::vector<std::uint8_t> cells_;
}; /* End of class Canvas */

} /* End of namespace terion::render */

#endif /* TERION_RENDER_CANVAS_H */
