/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#ifndef TERION_RENDER_PAINTER_H
#define TERION_RENDER_PAINTER_H

#include <string_view>
#include <terion/render/screen_buffer.h>

namespace terion {

class Painter
{
public:
    explicit Painter(ScreenBuffer& buffer) : buffer_(buffer) {}
    void put(int x, int y, std::string_view grapheme, TextStyle style = {}) const;
    void text(int x, int y, std::string_view text, TextStyle style = {}) const;
    void fill(Rect rect, Cell cell = {}) const;

private:
    ScreenBuffer& buffer_;
    void eraseGlyphAt(int x, int y) const;
}; /* End of class Painter */

} /* End of namespace terion */

#endif /* TERION_RENDER_PAINTER_H */
