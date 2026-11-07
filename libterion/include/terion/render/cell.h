/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#ifndef TERION_RENDER_CELL_H
#define TERION_RENDER_CELL_H

#include <string>
#include <terion/render/color.h>

namespace terion {

struct Cell
{
    std::string grapheme = " ";
    TextStyle style{};
    // 0 = continuation of a double-column glyph; 1 or 2 = visible width.
    unsigned char width = 1;
    bool operator==(const Cell&) const = default;
}; /* End of struct Cell */

} /* End of namespace terion */

#endif /* TERION_RENDER_CELL_H */
