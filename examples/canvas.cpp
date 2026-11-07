/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <iostream>
#include <terion/terion.h>

using namespace terion;

int main()
{
    render::Canvas canvas(72,32);
    canvas.rectangle({.x = 1,.y = 1,.width = 70,.height = 30});
    canvas.circle({.x = 36,.y = 16},12);
    canvas.line({.x = 3,.y = 28},{.x = 68,.y = 4});
    ScreenBuffer buffer({.width = 36,.height = 8});
    Painter painter(buffer);
    canvas.paint(painter);
    for (int y = 0; y < buffer.size().height; ++y)
    {
        for (int x = 0; x < buffer.size().width; ++x)
            std::cout << buffer.at(x,y).grapheme;
        std::cout << '\n';
    }
}
