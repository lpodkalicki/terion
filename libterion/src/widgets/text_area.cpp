/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <terion/render_node.h>
#include <terion/text_editing.h>
#include <algorithm>
#include <string_view>

using namespace terion;

void RenderNode::setTextCursor(Point position)
{
    if (type_ == Component::Type::TextField || type_ == Component::Type::NumberField ||
        type_ == Component::Type::PasswordField)
    {
        setCursorFromX(position.x);
        selection_active_ = false;
        edit_kind_ = EditKind::None;
        return;
    }
    if (type_ != Component::Type::TextArea) return;
    const int width = std::max(1,bounds_.width-2);
    const auto lines = detail::visualTextLines(text_,width,text_area_soft_wrap_);
    const int row = std::clamp(position.y-bounds_.y-1+text_area_first_row_,
        0,static_cast<int>(lines.size())-1);
    const int column = std::max(0,position.x-bounds_.x-1+
        (text_area_soft_wrap_ ? 0 : text_area_first_column_));
    cursor_byte_ = detail::visualOffset(text_,lines[static_cast<std::size_t>(row)],column);
    selection_active_ = false;
    text_area_follow_cursor_ = true;
    text_area_preferred_column_ = -1;
    edit_kind_ = EditKind::None;
}

void RenderNode::extendTextAreaSelection(Point position)
{
    if (type_ != Component::Type::TextArea || disabled_) return;
    const int visible_rows = std::max(1,bounds_.height-2);
    const int row_in_view = position.y-bounds_.y-1;
    if (row_in_view < 0) (void)scrollTextArea(-1);
    else if (row_in_view >= visible_rows) (void)scrollTextArea(1);
    const auto lines = detail::visualTextLines(text_,std::max(1,bounds_.width-2),text_area_soft_wrap_);
    const int row = std::clamp(position.y-bounds_.y-1+text_area_first_row_,
        0,static_cast<int>(lines.size())-1);
    const int column = std::max(0,position.x-bounds_.x-1+
        (text_area_soft_wrap_ ? 0 : text_area_first_column_));
    if (!selection_active_) selection_anchor_ = cursor_byte_;
    cursor_byte_ = detail::visualOffset(text_,lines[static_cast<std::size_t>(row)],column);
    selection_active_ = selection_anchor_ != cursor_byte_;
    text_area_follow_cursor_ = true;
    text_area_preferred_column_ = -1;
}

bool RenderNode::scrollTextArea(int rows)
{
    if (type_ != Component::Type::TextArea) return false;
    const int width = std::max(1,bounds_.width-2);
    const auto lines = detail::visualTextLines(text_,width,text_area_soft_wrap_);
    if (text_area_soft_wrap_ && text_area_last_wrap_width_ != width &&
        text_area_last_wrap_width_ >= 0)
        text_area_first_row_ = static_cast<int>(detail::visualLineIndex(lines,text_area_top_byte_));
    const int visible_rows = std::max(1,bounds_.height-2);
    const int maximum = std::max(0,static_cast<int>(lines.size())-visible_rows);
    const int target = std::clamp(text_area_first_row_+rows,0,maximum);
    if (target == text_area_first_row_) return false;
    text_area_first_row_ = target;
    text_area_follow_cursor_ = false;
    if (text_area_soft_wrap_)
    {
        text_area_last_wrap_width_ = width;
        text_area_top_byte_ = lines[static_cast<std::size_t>(target)].begin;
    }
    return true;
}

void RenderNode::selectWordAt(Point position)
{
    if (disabled_) return;
    setTextCursor(position);
    const auto [begin,end] = detail::wordAt(text_,cursor_byte_);
    selection_anchor_ = begin;
    cursor_byte_ = end;
    selection_active_ = begin != end;
    text_area_preferred_column_ = -1;
}

void RenderNode::selectLineAt(Point position)
{
    if (disabled_) return;
    setTextCursor(position);
    if (type_ == Component::Type::TextArea)
    {
        const auto previous = cursor_byte_ == 0 ? std::string::npos :
            text_.rfind('\n',cursor_byte_-1);
        const auto next = text_.find('\n',cursor_byte_);
        selection_anchor_ = previous == std::string::npos ? 0 : previous+1;
        cursor_byte_ = next == std::string::npos ? text_.size() : next+1;
    }
    else
    {
        selection_anchor_ = 0;
        cursor_byte_ = text_.size();
    }
    selection_active_ = selection_anchor_ != cursor_byte_;
    text_area_preferred_column_ = -1;
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
    const auto lines = detail::visualTextLines(text_,content.width,text_area_soft_wrap_);
    const auto cursor_row = detail::visualLineIndex(lines,cursor_byte_);
    const int cursor_column = detail::visualColumn(text_,lines[cursor_row],cursor_byte_);
    if (text_area_soft_wrap_)
    {
        if (text_area_last_wrap_width_ >= 0 && text_area_last_wrap_width_ != content.width)
            text_area_first_row_ = static_cast<int>(detail::visualLineIndex(lines,text_area_top_byte_));
        text_area_last_wrap_width_ = content.width;
        text_area_first_column_ = 0;
    }
    text_area_first_row_ = std::clamp(text_area_first_row_,0,
        std::max(0,static_cast<int>(lines.size())-content.height));
    if (focused && text_area_follow_cursor_)
    {
        if (static_cast<int>(cursor_row) < text_area_first_row_)
            text_area_first_row_ = static_cast<int>(cursor_row);
        else if (static_cast<int>(cursor_row) >= text_area_first_row_+content.height)
            text_area_first_row_ = static_cast<int>(cursor_row)-content.height+1;
        if (!text_area_soft_wrap_)
        {
            if (cursor_column < text_area_first_column_) text_area_first_column_ = cursor_column;
            else if (cursor_column >= text_area_first_column_+content.width)
                text_area_first_column_ = cursor_column-content.width+1;
        }
    }
    if (text_area_soft_wrap_)
        text_area_top_byte_ = lines[static_cast<std::size_t>(text_area_first_row_)].begin;
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
                const auto cluster = nextGrapheme(text_,i);
                if (!cluster.bytes) break;
                const int width = cluster.columns;
                if (column >= text_area_first_column_ &&
                    column+width <= text_area_first_column_+content.width)
                {
                    TextStyle style = normal;
                    if (selection_active_ && i >= std::min(selection_anchor_,cursor_byte_) &&
                        i < std::max(selection_anchor_,cursor_byte_))
                    {
                        style.background = palette.primary;
                        style.foreground = palette.on_primary;
                    }
                    painter.put(content.x+column-text_area_first_column_,content.y+row,
                        std::string_view(text_).substr(i,cluster.bytes),style);
                }
                column += width;
                i += cluster.bytes;
            }
        }
    }
    if (focused && cursor_column >= text_area_first_column_ &&
        cursor_column < text_area_first_column_+content.width &&
        static_cast<int>(cursor_row) >= text_area_first_row_ &&
        static_cast<int>(cursor_row) < text_area_first_row_+content.height)
    {
        TextStyle caret = normal;
        caret.background = palette.primary;
        caret.foreground = palette.on_primary;
        painter.styleAt(content.x+cursor_column-text_area_first_column_,
            content.y+static_cast<int>(cursor_row)-text_area_first_row_,caret);
    }
}
