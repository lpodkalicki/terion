/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <terion/render/differential_renderer.h>
#include <terion/grapheme.h>
#include <terion/emoji_compat.h>
#include <algorithm>
#include <string>
#include <vector>

using namespace terion;

static std::string styleSequence(const TextStyle& s)
{
    return "\033[0;" + std::string(s.bold ? "1;" : "") + (s.underline ? "4;" : "") + (s.italic ? "3;" : "") +
        (s.strikethrough ? "9;" : "") + (s.dim ? "2;" : "") +
        "38;2;" + std::to_string(s.foreground.red) + ";" + std::to_string(s.foreground.green) + ";" + std::to_string(s.foreground.blue) +
        ";48;2;" + std::to_string(s.background.red) + ";" + std::to_string(s.background.green) + ";" + std::to_string(s.background.blue) + "m";

}

void DifferentialRenderer::render(const ScreenBuffer& current, TerminalBackend& backend)
{
    const auto size = current.size();
    const bool full = !previous_ || previous_->size() != size;
    const bool emoji_compatibility = backend.capabilities().emoji_compatibility;
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

        // The terminal can disagree with our cell widths for emoji and characters
        // with East Asian Ambiguous width (for example the middle dot or box lines).
        // When such a row changes, the previous frame is not a reliable description
        // of its physical cells: clear and paint it from a known cursor position.
        // This also removes remnants after deleting or replacing wide graphemes.
        const auto nonAscii = [](const Cell& cell) {
            // A grapheme can start with ASCII and still contain Unicode
            // (for example e + U+0301 COMBINING ACUTE ACCENT).
            return std::any_of(cell.grapheme.begin(),cell.grapheme.end(),
                [](unsigned char c) { return c >= 0x80; });
        };
        bool repaint_row = false;
        if (!full)
        {
            bool changed = false;
            bool has_unicode = false;
            bool changed_wide = false;
            for (int x = 0; x < size.width; ++x)
            {
                const auto& now = current.at(x,y);
                const auto& before = previous_->at(x,y);
                changed |= dirty[x];
                has_unicode |= nonAscii(now) || nonAscii(before);
                changed_wide |= dirty[x] && (now.width != 1 || before.width != 1);
            }
            repaint_row = changed && (has_unicode || changed_wide);
        }
        if (repaint_row)
        {
            output += "\033[" + std::to_string(y+1) + ";1H\033[2K";
            std::fill(dirty.begin(),dirty.end(),true);
            style.reset();
            cursor = {.x = -1,.y = -1};
        }

        // A continuation cell cannot be drawn independently. Account for both frames:
        // a wide glyph may have been removed as well as introduced.
        for (int x = 1; x < size.width; ++x)
        {
            if (dirty[x] && (current.at(x,y).width == 0 ||
                (!full && previous_->at(x,y).width == 0))) dirty[x-1] = true;
        }

        for (int x = 0; x < size.width;)
        {
            const auto& cell = current.at(x,y);
            if (!dirty[x] || cell.width == 0) { ++x; continue; }

            // Vertical separators are structural alignment landmarks. Never
            // reach them by predicted cursor advance: a terminal may draw a
            // preceding grapheme (especially an emoji sequence) with a width
            // different from our cell model. Re-anchor *before* the separator,
            // not only after earlier Unicode output.
            const bool vertical_separator = cell.grapheme == "│" || cell.grapheme == "┃" ||
                cell.grapheme == "║" || cell.grapheme == "┆" || cell.grapheme == "┊";
            if (vertical_separator || cursor.x != x || cursor.y != y)
            {
                output += "\033[" + std::to_string(y+1) + ";" + std::to_string(x+1) + "H";
            }
            if (!style || *style != cell.style)
            {
                output += styleSequence(cell.style);
                style = cell.style;
            }

            if (emoji_compatibility && cell.grapheme.size() > 3)
                output += compatibleTerminalGrapheme(cell.grapheme,cell.width);
            else
                output += cell.grapheme;
            // A single non-ASCII codepoint can also be ambiguous-width in a
            // terminal (e.g. U+00B7 MIDDLE DOT and U+2502 BOX DRAWINGS).
            // Re-anchor after *every* non-ASCII grapheme, not only multi-codepoint
            // emoji and two-cell glyphs. The following ASCII and separators then
            // start at their actual, absolute screen columns.
            const bool uncertain_advance = cell.width != 1 || nonAscii(cell);
            cursor = {.x = uncertain_advance ? -1 : x+cell.width,.y = y};
            x += cell.width;
        }
    }
    if (!output.empty()) { backend.write(output); backend.flush(); }
    previous_ = current;
}

