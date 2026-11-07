/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <terion/render/differential_renderer.h>
#include <string>

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
    for (int y = 0; y < size.height; ++y)
    {
        for (int x = 0; x < size.width;)
        {
            const auto& cell = current.at(x,y);
            if (cell.width == 0 || (!full && cell == previous_->at(x,y))) { ++x; continue; }
            const int start = x;
            std::string run;
            while (x < size.width)
            {
                const auto& next = current.at(x,y);
                if (next.width == 0) { ++x; continue; }
                if (!full && next == previous_->at(x,y)) break;
                if (style && *style != next.style && !run.empty()) break;
                if (!style || *style != next.style) { output += "\033[" + std::to_string(y+1) + ";" + std::to_string(x+1) + "H" + styleSequence(next.style); style = next.style; }
                else if (run.empty()) output += "\033[" + std::to_string(y+1) + ";" + std::to_string(x+1) + "H";
                run += next.grapheme;
                x += next.width;
            }
            output += run;
            if (x == start) ++x;
        }
    }
    if (!output.empty()) { backend.write(output); backend.flush(); }
    previous_ = current;
}

