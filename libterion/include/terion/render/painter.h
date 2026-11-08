/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#ifndef TERION_RENDER_PAINTER_H
#define TERION_RENDER_PAINTER_H

#include <optional>
#include <string_view>
#include <vector>
#include <terion/render/screen_buffer.h>

namespace terion {

class Painter
{
public:
    class ClipScope
    {
    public:
        ClipScope(Painter& painter, Rect rect) : painter_(painter) { painter_.pushClip(rect); }
        ~ClipScope() { painter_.popClip(); }
        ClipScope(const ClipScope&) = delete;
        ClipScope& operator=(const ClipScope&) = delete;

    private:
        Painter& painter_;
    }; /* End of class ClipScope */

    explicit Painter(ScreenBuffer& buffer) : buffer_(buffer) {}
    void pushClip(Rect rect);
    void popClip();
    void put(int x, int y, std::string_view grapheme, TextStyle style = {}) const;
    void text(int x, int y, std::string_view text, TextStyle style = {}) const;
    void fill(Rect rect, Cell cell = {}) const;
    void fillBackground(Rect rect, Color color) const;
    void styleAt(int x, int y, TextStyle style) const;
    void horizontalLine(int x, int y, int length, TextStyle style = {}) const;
    void verticalLine(int x, int y, int length, TextStyle style = {}) const;
    void line(Point start, Point end, TextStyle style = {}) const;
    void border(Rect rect, BorderStyle border = BorderStyle::Single, TextStyle style = {}) const;

private:
    ScreenBuffer& buffer_;
    std::optional<Rect> clip_;
    std::vector<std::optional<Rect>> clip_stack_;
    [[nodiscard]] bool visible(int x, int y) const noexcept;
    void eraseGlyphAt(int x, int y) const;
}; /* End of class Painter */

} /* End of namespace terion */

#endif /* TERION_RENDER_PAINTER_H */
