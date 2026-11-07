/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <terion/render/differential_renderer.h>
#include <string>
#include <vector>

using namespace terion;

static std::string styleSequence(const TextStyle& s)
{
    return "\033[0;" + std::string(s.bold ? "1;" : "") + (s.underline ? "4;" : "") +
        "38;2;" + std::to_string(s.foreground.red) + ";" + std::to_string(s.foreground.green) + ";" + std::to_string(s.foreground.blue) +
        ";48;2;" + std::to_string(s.background.red) + ";" + std::to_string(s.background.green) + ";" + std::to_string(s.background.blue) + "m";

}

void DifferentialRenderer::render(const ScreenBuffer& current, TerminalBackend& backend)
{
    const auto size = current.size();
    const bool full = !previous_ || previous_->size() != size;
    std::string output;
    if (full) output += "\033[2J";
    std::optional<TextStyle> style;
    Point cursor {.x = -1,.y = -1};

    for (int y = 0; y < size.height; ++y)
    {
        std::vector<bool> dirty(static_cast<std::size_t>(size.width), full);
        if (!full)
        {
            for (int x = 0; x < size.width; ++x)
            {
                dirty[x] = current.at(x,y) != previous_->at(x,y);
            }
        }

        // A continuation cell cannot be drawn independently. Repaint its leading cell.
        for (int x = 1; x < size.width; ++x)
        {
            if (current.at(x,y).width == 0 && dirty[x]) dirty[x-1] = true;
        }

        for (int x = 0; x < size.width;)
        {
            const auto& cell = current.at(x,y);
            if (!dirty[x] || cell.width == 0) { ++x; continue; }

            if (cursor.x != x || cursor.y != y)
            {
                output += "\033[" + std::to_string(y+1) + ";" + std::to_string(x+1) + "H";
            }
            if (!style || *style != cell.style)
            {
                output += styleSequence(cell.style);
                style = cell.style;
            }

            output += cell.grapheme;
            cursor = {.x = x + cell.width,.y = y};
            x += cell.width;
        }
    }
    if (!output.empty()) { backend.write(output); backend.flush(); }
    previous_ = current;
}

