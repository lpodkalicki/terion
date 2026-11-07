/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#ifndef TERION_GEOMETRY_H
#define TERION_GEOMETRY_H

namespace terion {

struct Point
{
    int x = 0;
    int y = 0;
    bool operator==(const Point&) const = default;
}; /* End of struct Point */

struct Size
{
    int width = 0;
    int height = 0;
    bool operator==(const Size&) const = default;
}; /* End of struct Size */

struct Rect
{
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
}; /* End of struct Rect */

} /* End of namespace terion */

#endif /* TERION_GEOMETRY_H */
