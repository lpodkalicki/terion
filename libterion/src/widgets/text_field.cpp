/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <terion/render_node.h>
#include <terion/utf8.h>
#include <terion/text_editor_core.h>
#include <terion/text_editing.h>
#include <terion/clipboard.h>
#include <algorithm>
#include <charconv>
#include <cmath>
#include <limits>

using namespace terion;

using terion::detail::previousBoundary;
using terion::detail::nextBoundary;
using terion::detail::encodeUtf8;

void RenderNode::recordEdit(EditKind kind)
{
    const bool merge = kind != EditKind::Other && kind != EditKind::None &&
        kind == edit_kind_ && cursor_byte_ == last_edit_cursor_ &&
        !selection_active_ && !undo_stack_.empty();
    if (!merge)
    {
        if (undo_stack_.size() >= max_history_) undo_stack_.erase(undo_stack_.begin());
        undo_stack_.push_back({text_,cursor_byte_,selection_anchor_,selection_active_});
    }
    redo_stack_.clear();
    edit_kind_ = kind;
}

bool RenderNode::restoreEdit(bool redo)
{
    auto& source = redo ? redo_stack_ : undo_stack_;
    auto& destination = redo ? undo_stack_ : redo_stack_;
    edit_kind_ = EditKind::None;
    if (source.empty()) return true;
    if (destination.size() >= max_history_) destination.erase(destination.begin());
    destination.push_back({text_,cursor_byte_,selection_anchor_,selection_active_});
    auto state = std::move(source.back());
    source.pop_back();
    text_ = std::move(state.text);
    cursor_byte_ = state.cursor;
    selection_anchor_ = state.anchor;
    selection_active_ = state.selected;
    text_area_preferred_column_ = -1;
    text_area_follow_cursor_ = true;
    if (on_change_) on_change_(text_);
    return true;
}

bool RenderNode::handleTextInput(const Event& event)
{
    if (type_ != Component::Type::TextField && type_ != Component::Type::TextArea &&
        type_ != Component::Type::NumberField && type_ != Component::Type::PasswordField) return false;
    if (disabled_) return false;
    const bool number_field = type_ == Component::Type::NumberField;
    if (number_field && (event.key == Key::Up || event.key == Key::Down))
    {
        if (read_only_) return true;
        double current = 0.0;
        const auto [end,err] = std::from_chars(text_.data(),text_.data()+text_.size(),current);
        if (err != std::errc{} || end != text_.data()+text_.size() || !std::isfinite(current)) current = 0.0;
        const double delta = event.key == Key::Up ? number_step_ : -number_step_;
        const double next = std::clamp(current+delta,number_min_,number_max_);
        if (!std::isfinite(next)) return true;
        recordEdit(EditKind::Other);
        text_ = std::to_string(next);
        while (text_.find('.') != std::string::npos && text_.back() == '0') text_.pop_back();
        if (!text_.empty() && text_.back() == '.') text_.pop_back();
        cursor_byte_ = text_.size();
        selection_active_ = false;
        last_edit_cursor_ = cursor_byte_;
        if (on_change_) on_change_(text_);
        return true;
    }
    const bool multiline = type_ == Component::Type::TextArea;
    if (multiline && event.key != Key::PageUp && event.key != Key::PageDown &&
        (event.key == Key::Left || event.key == Key::Right || event.key == Key::Up ||
         event.key == Key::Down || event.key == Key::Home || event.key == Key::End ||
         event.key == Key::Backspace || event.key == Key::Delete || event.key == Key::Enter ||
         (event.key == Key::Character && !event.ctrl && !event.alt) ||
         event.key == Key::Paste))
        text_area_follow_cursor_ = true;
    const auto remove_selection = [&]() {
        if (!selection_active_ || selection_anchor_ == cursor_byte_) return false;
        const auto start = std::min(selection_anchor_,cursor_byte_);
        const auto end = std::max(selection_anchor_,cursor_byte_);
        text_.erase(start,end-start);
        cursor_byte_ = start;
        selection_active_ = false;
        return true;
    };
    const auto move_cursor = [&](std::size_t new_cursor) {
        if (event.shift)
        {
            if (!selection_active_) selection_anchor_ = cursor_byte_;
            selection_active_ = true;
        }
        else selection_active_ = false;
        edit_kind_ = EditKind::None;
        cursor_byte_ = new_cursor;
    };
    if (event.ctrl && !event.alt && event.key == Key::Character &&
        (event.character == U'z' || event.character == U'Z' ||
         event.character == U'y' || event.character == U'Y'))
    {
        if (read_only_) return true;
        const bool redo = event.character == U'y' || event.character == U'Y' || event.shift;
        return restoreEdit(redo);
    }
    // macOS Terminal.app and other terminals may encode Option+Left/Right as
    // Escape+b / Escape+f instead of modified CSI arrow sequences.
    if (event.alt && !event.ctrl && event.key == Key::Character &&
        (event.character == U'b' || event.character == U'f'))
    {
        move_cursor(event.character == U'b' ? detail::previousWordBoundary(text_,cursor_byte_) :
            detail::nextWordBoundary(text_,cursor_byte_));
        text_area_follow_cursor_ = true;
        text_area_preferred_column_ = -1;
        return true;
    }
    if (event.ctrl && event.key == Key::Character && (event.character == U'a' || event.character == U'A'))
    {
        edit_kind_ = EditKind::None;
        selection_anchor_ = 0;
        cursor_byte_ = text_.size();
        selection_active_ = true;
        return true;
    }
    // Clipboard shortcuts operate on UTF-8 byte boundaries maintained by the editor.
    // Never export PasswordField contents, even when the password is revealed.
    if (event.ctrl && !event.alt && event.key == Key::Character)
    {
        const auto ch = event.character;
        if (ch == U'c' || ch == U'C' || ch == U'x' || ch == U'X')
        {
            if (type_ == Component::Type::PasswordField) return true;
            if (selection_active_ && selection_anchor_ != cursor_byte_)
            {
                const auto begin = std::min(selection_anchor_,cursor_byte_);
                const auto end = std::max(selection_anchor_,cursor_byte_);
                Clipboard::set(std::string_view(text_).substr(begin,end-begin));
                if ((ch == U'x' || ch == U'X') && !read_only_)
                {
                    recordEdit(EditKind::Other);
                    (void)remove_selection();
                    last_edit_cursor_ = cursor_byte_;
            if (on_change_) on_change_(text_);
                }
            }
            return true;
        }
        if (ch == U'v' || ch == U'V')
        {
            Event paste {.key = Key::Paste,.text = Clipboard::get()};
            return handleTextInput(paste);
        }
    }
    if (multiline && (event.key == Key::Up || event.key == Key::Down))
    {
        const auto lines = detail::visualTextLines(text_,std::max(1,bounds_.width-2),text_area_soft_wrap_);
        const auto current = detail::visualLineIndex(lines,cursor_byte_);
        if (text_area_preferred_column_ < 0)
            text_area_preferred_column_ = detail::visualColumn(text_,lines[current],cursor_byte_);
        const auto target = event.key == Key::Up ? (current == 0 ? 0 : current-1) :
            std::min(current+1,lines.size()-1);
        move_cursor(detail::visualOffset(text_,lines[target],text_area_preferred_column_));
        return true;
    }
    if (event.key != Key::Up && event.key != Key::Down) text_area_preferred_column_ = -1;
    if (multiline && event.key == Key::Enter)
    {
        if (read_only_) return true;
        recordEdit(EditKind::Other);
        (void)remove_selection();
        text_.insert(cursor_byte_,1,'\n');
        ++cursor_byte_;
        last_edit_cursor_ = cursor_byte_;
            if (on_change_) on_change_(text_);
        return true;
    }
    if (multiline && event.key == Key::PageUp)
    {
        (void)scrollTextArea(-std::max(1,bounds_.height-2));
        return true;
    }
    if (multiline && event.key == Key::PageDown)
    {
        (void)scrollTextArea(std::max(1,bounds_.height-2));
        return true;
    }
    if (multiline && (event.key == Key::Home || event.key == Key::End) && !event.ctrl)
    {
        const auto lines = detail::visualTextLines(text_,std::max(1,bounds_.width-2),text_area_soft_wrap_);
        const auto current = detail::visualLineIndex(lines,cursor_byte_);
        move_cursor(event.key == Key::Home ? lines[current].begin : lines[current].end);
        return true;
    }
    if (number_field && !read_only_ && (event.key == Key::Character || event.key == Key::Paste))
    {
        std::string inserted;
        if (event.key == Key::Character)
        {
            if (event.ctrl || event.alt || event.character > 127) return true;
            inserted = static_cast<char>(event.character);
        }
        else inserted = detail::sanitizePaste(event.text,false);
        if (inserted.empty()) return true;
        if (inserted.find_first_not_of("0123456789+-.eE") != std::string::npos) return true;
        std::string candidate = text_;
        const auto first = selection_active_ ? std::min(selection_anchor_,cursor_byte_) : cursor_byte_;
        const auto last = selection_active_ ? std::max(selection_anchor_,cursor_byte_) : cursor_byte_;
        candidate.replace(first,last-first,inserted);
        const auto exponent = candidate.find_first_of("eE");
        const auto significand = candidate.substr(0,exponent);
        const bool valid_sign = significand.find_first_of("+-",1) == std::string::npos;
        const bool valid_dot = significand.find('.') == significand.rfind('.');
        const bool valid_exponent = exponent == std::string::npos ||
            (candidate.find_first_of("eE",exponent+1) == std::string::npos &&
             candidate.substr(exponent+1).find_first_of("+-",1) == std::string::npos &&
             candidate.substr(exponent+1).find('.') == std::string::npos);
        if (!valid_sign || !valid_dot || !valid_exponent) return true;
    }
    switch (event.key)
    {
        case Key::Paste:
        {
            if (read_only_) return true;
            const auto sanitized = detail::sanitizePaste(event.text,multiline);
            if (sanitized.empty()) return true;
            recordEdit(EditKind::Other);
            (void)remove_selection();
            text_.insert(cursor_byte_,sanitized);
            cursor_byte_ += sanitized.size();
            last_edit_cursor_ = cursor_byte_;
            if (on_change_) on_change_(text_);
            return true;
        }
        case Key::Left:
            move_cursor(event.ctrl || event.alt ? detail::previousWordBoundary(text_,cursor_byte_) :
                detail::previousBoundary(text_,cursor_byte_));
            return true;
        case Key::Right:
            move_cursor(event.ctrl || event.alt ? detail::nextWordBoundary(text_,cursor_byte_) :
                detail::nextBoundary(text_,cursor_byte_));
            return true;
        case Key::Home: move_cursor(0); return true;
        case Key::End: move_cursor(text_.size()); return true;
        case Key::Backspace:
        {
            if (read_only_) return true;
            if ((!selection_active_ || selection_anchor_ == cursor_byte_) && cursor_byte_ == 0) return true;
            recordEdit((event.ctrl || event.alt || (selection_active_ && selection_anchor_ != cursor_byte_)) ?
                EditKind::Other : EditKind::Backspace);
            if (!remove_selection())
            {
                if (cursor_byte_ == 0) return true;
                const auto previous = event.ctrl || event.alt ?
                    detail::previousWordBoundary(text_,cursor_byte_) :
                    detail::previousBoundary(text_,cursor_byte_);
                text_.erase(previous,cursor_byte_-previous);
                cursor_byte_ = previous;
            }
            last_edit_cursor_ = cursor_byte_;
            if (on_change_) on_change_(text_);
            return true;
        }
        case Key::Delete:
        {
            if (read_only_) return true;
            if ((!selection_active_ || selection_anchor_ == cursor_byte_) && cursor_byte_ == text_.size()) return true;
            recordEdit((event.ctrl || event.alt || (selection_active_ && selection_anchor_ != cursor_byte_)) ?
                EditKind::Other : EditKind::Delete);
            if (!remove_selection())
            {
                if (cursor_byte_ == text_.size()) return true;
                text_.erase(cursor_byte_,(event.ctrl || event.alt ?
                    detail::nextWordBoundary(text_,cursor_byte_) :
                    detail::nextBoundary(text_,cursor_byte_))-cursor_byte_);
            }
            last_edit_cursor_ = cursor_byte_;
            if (on_change_) on_change_(text_);
            return true;
        }
        case Key::Character:
        {
            if (event.character < 32 || event.character == 127 || event.ctrl || event.alt) return false;
            if (read_only_) return true;
            const auto encoded = encodeUtf8(event.character);
            recordEdit(selection_active_ && selection_anchor_ != cursor_byte_ ? EditKind::Other : EditKind::Typing);
            (void)remove_selection();
            text_.insert(cursor_byte_,encoded);
            cursor_byte_ += encoded.size();
            last_edit_cursor_ = cursor_byte_;
            if (on_change_) on_change_(text_);
            return true;
        }
        default: return false;
    }
}

void RenderNode::setCursorFromX(const int x)
{
    if (type_ != Component::Type::TextField && type_ != Component::Type::PasswordField &&
        type_ != Component::Type::NumberField) return;
    const int inset = text_field_variant_ == TextFieldVariant::Outlined ? 2 : 1;
    const int desired = std::max(0,x - bounds_.x - inset + scroll_columns_);
    const bool masked = type_ == Component::Type::PasswordField && !password_reveal_;
    int columns = 0;
    cursor_byte_ = 0;
    while (cursor_byte_ < text_.size())
    {
        const auto cluster = nextGrapheme(text_,cursor_byte_);
        if (!cluster.bytes || columns + (masked ? 1 : cluster.columns) > desired) break;
        columns += masked ? 1 : cluster.columns;
        cursor_byte_ += cluster.bytes;
    }
}

void RenderNode::extendTextFieldSelection(const Point position)
{
    if ((type_ != Component::Type::TextField && type_ != Component::Type::NumberField &&
        type_ != Component::Type::PasswordField) || disabled_) return;
    const auto previous = cursor_byte_;
    if (!selection_active_) selection_anchor_ = previous;
    setCursorFromX(position.x);
    selection_active_ = selection_anchor_ != cursor_byte_;
}
