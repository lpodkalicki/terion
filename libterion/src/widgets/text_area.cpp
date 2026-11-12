/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <terion/render_node.h>
#include <terion/text_editor_core.h>
#include <terion/utf8.h>
#include <algorithm>
#include <string_view>
#include <vector>

using namespace terion;

namespace {

struct Line
{
    std::size_t begin;
    std::size_t end;
};

std::vector<Line> linesOf(std::string_view text)
{
    std::vector<Line> lines;
    std::size_t start = 0;
    for (std::size_t i = 0; i < text.size(); ++i)
    {
        if (text[i] != '\n') continue;
        lines.push_back({start,i});
        start = i+1;
    }
    lines.push_back({start,text.size()});
    return lines;
}

std::size_t lineIndex(const std::vector<Line>& lines, std::size_t cursor)
{
    for (std::size_t i = 0; i+1 < lines.size(); ++i)
        if (cursor <= lines[i].end) return i;
    return lines.size()-1;
}

std::size_t columnOffset(std::string_view text, Line line, int column)
{
    int current_column = 0;
    std::size_t cursor = line.begin;
    while (cursor < line.end)
    {
        const auto decoded = decodeUtf8(text,cursor);
        if (!decoded.bytes || current_column + codePointWidth(decoded.code_point) > column) break;
        current_column += codePointWidth(decoded.code_point);
        cursor += decoded.bytes;
    }
    return cursor;
}

int cursorColumn(std::string_view text, Line line, std::size_t cursor)
{
    int column = 0;
    for (std::size_t i = line.begin; i < std::min(line.end,cursor);)
    {
        const auto decoded = decodeUtf8(text,i);
        if (!decoded.bytes) break;
        column += codePointWidth(decoded.code_point);
        i += decoded.bytes;
    }
    return column;
}

} /* End of anonymous namespace */

void RenderNode::setTextCursor(Point position)
{
    if (type_ == Component::Type::TextField) { setCursorFromX(position.x); return; }
    if (type_ != Component::Type::TextArea) return;
    const auto lines = linesOf(text_);
    const int row = std::clamp(position.y-bounds_.y-1+text_area_first_row_,0,static_cast<int>(lines.size())-1);
    const int column = std::max(0,position.x-bounds_.x-1+text_area_first_column_);
    cursor_byte_ = columnOffset(text_,lines[static_cast<std::size_t>(row)],column);
    selection_active_ = false;
    text_area_preferred_column_ = -1;
}

bool RenderNode::scrollTextArea(int rows)
{
    if (type_ != Component::Type::TextArea) return false;
    const int visible_rows = std::max(1,bounds_.height-2);
    const int maximum = std::max(0,static_cast<int>(linesOf(text_).size())-visible_rows);
    const int target = std::clamp(text_area_first_row_+rows,0,maximum);
    if (target == text_area_first_row_) return false;
    text_area_first_row_ = target;
    return true;
}

void RenderNode::paintTextArea(Painter& painter, std::string_view focused_id, const Theme* theme) const
{
    if (bounds_.width <= 0 || bounds_.height <= 0) return;
    const Palette palette = theme ? theme->palette : Theme::dark().palette;
    const bool focused = focused_id == std::to_string(id_) && !disabled_;
    const Color background = palette.surface;
    const TextStyle normal {.foreground = disabled_ ? palette.muted : palette.foreground,
        .background = background};
    painter.fillBackground(bounds_,background);
    TextStyle outline = normal;
    outline.foreground = focused ? palette.primary : palette.muted;
    if (bounds_.width >= 2 && bounds_.height >= 2)
        painter.border(bounds_,focused ? BorderStyle::Heavy : BorderStyle::Rounded,outline);
    const Rect content {.x = bounds_.x+1,.y = bounds_.y+1,
        .width = std::max(0,bounds_.width-2),.height = std::max(0,bounds_.height-2)};
    if (content.width == 0 || content.height == 0) return;
    const auto lines = linesOf(text_);
    const auto cursor_row = lineIndex(lines,cursor_byte_);
    const int cursor_column = cursorColumn(text_,lines[cursor_row],cursor_byte_);
    if (focused)
    {
        if (static_cast<int>(cursor_row) < text_area_first_row_)
            text_area_first_row_ = static_cast<int>(cursor_row);
        else if (static_cast<int>(cursor_row) >= text_area_first_row_+content.height)
            text_area_first_row_ = static_cast<int>(cursor_row)-content.height+1;
        if (cursor_column < text_area_first_column_) text_area_first_column_ = cursor_column;
        else if (cursor_column >= text_area_first_column_+content.width)
            text_area_first_column_ = cursor_column-content.width+1;
    }
    Painter::ClipScope clip(painter,content);
    if (text_.empty() && !focused)
    {
        TextStyle hint = normal;
        hint.foreground = palette.muted;
        painter.text(content.x,content.y,placeholder_,hint);
    }
    else
    {
        for (int row = 0; row < content.height; ++row)
        {
            const int index = text_area_first_row_+row;
            if (index >= static_cast<int>(lines.size())) break;
            const auto line = lines[static_cast<std::size_t>(index)];
            int column = 0;
            for (std::size_t i = line.begin; i < line.end;)
            {
                const auto decoded = decodeUtf8(text_,i);
                if (!decoded.bytes) break;
                const int width = codePointWidth(decoded.code_point);
                if (column >= text_area_first_column_ && column+width <= text_area_first_column_+content.width)
                {
                    TextStyle style = normal;
                    if (selection_active_ && i >= std::min(selection_anchor_,cursor_byte_) &&
                        i < std::max(selection_anchor_,cursor_byte_))
                    {
                        style.background = palette.primary;
                        style.foreground = palette.on_primary;
                    }
                    painter.put(content.x+column-text_area_first_column_,content.y+row,
                        std::string_view(text_).substr(i,decoded.bytes),style);
                }
                column += width;
                i += decoded.bytes;
            }
        }
    }
    if (focused)
    {
        TextStyle caret = normal;
        caret.background = palette.primary;
        caret.foreground = palette.on_primary;
        painter.styleAt(content.x+cursor_column-text_area_first_column_,
            content.y+static_cast<int>(cursor_row)-text_area_first_row_,caret);
    }
}
