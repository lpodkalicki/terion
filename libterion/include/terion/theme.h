/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#ifndef TERION_THEME_H
#define TERION_THEME_H

#include <terion/render/color.h>

namespace terion {

struct Palette
{
    Color primary = Color::rgb(90,160,255);
    Color on_primary = Color::rgb(0,0,0);
    Color foreground = Color::rgb(235,235,240);
    Color background = Color::rgb(20,22,28);
    Color surface = Color::rgb(38,42,52);
    Color muted = Color::rgb(105,110,120);
}; /* End of struct Palette */

struct Spacing
{
    int unit = 1;
    constexpr int operator()(int multiplier) const { return unit * multiplier; }
}; /* End of struct Spacing */

struct Theme
{
    Palette palette = {};
    Spacing spacing = {};

    static constexpr Theme dark() { return {}; }
    static constexpr Theme light()
    {
        return {.palette = {.primary = Color::rgb(35,96,205),
                            .on_primary = Color::rgb(255,255,255),
                            .foreground = Color::rgb(30,35,42),
                            .background = Color::rgb(248,249,251),
                            .surface = Color::rgb(228,231,238),
                            .muted = Color::rgb(115,120,130)}};
    }
}; /* End of struct Theme */

} /* End of namespace terion */

#endif /* TERION_THEME_H */
