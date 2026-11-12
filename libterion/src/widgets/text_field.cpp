/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <terion/render_node.h>
#include <terion/utf8.h>
#include <terion/text_editor_core.h>
#include <algorithm>
#include <charconv>
#include <cmath>
#include <limits>

using namespace terion;

using terion::detail::previousBoundary;
using terion::detail::nextBoundary;
using terion::detail::encodeUtf8;

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
        text_ = std::to_string(next);
        while (text_.find('.') != std::string::npos && text_.back() == '0') text_.pop_back();
        if (!text_.empty() && text_.back() == '.') text_.pop_back();
        cursor_byte_ = text_.size();
        selection_active_ = false;
        if (on_change_) on_change_(text_);
        return true;
    }
    const bool multiline = type_ == Component::Type::TextArea;
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
        if (multiline && event.shift)
        {
            if (!selection_active_) selection_anchor_ = cursor_byte_;
            selection_active_ = true;
        }
        else selection_active_ = false;
        cursor_byte_ = new_cursor;
    };
    if (event.ctrl && event.key == Key::Character && (event.character == U'a' || event.character == U'A'))
    {
        selection_anchor_ = 0;
        cursor_byte_ = text_.size();
        selection_active_ = true;
        return true;
    }
    if (multiline && (event.key == Key::Up || event.key == Key::Down))
    {
        const std::size_t start = text_.rfind('\n',cursor_byte_ == 0 ? 0 : cursor_byte_-1);
        const std::size_t line_begin = start == std::string::npos ? 0 : start+1;
        const std::size_t ending = text_.find('\n',cursor_byte_);
        const std::size_t line_end = ending == std::string::npos ? text_.size() : ending;
        if (text_area_preferred_column_ < 0)
            {
                int column = 0;
                for (std::size_t i = line_begin; i < cursor_byte_;)
                {
                    const auto decoded = decodeUtf8(text_,i);
                    if (!decoded.bytes) break;
                    column += codePointWidth(decoded.code_point);
                    i += decoded.bytes;
                }
                text_area_preferred_column_ = column;
            }
        std::size_t target_begin = line_begin;
        std::size_t target_end = line_end;
        if (event.key == Key::Up)
        {
            if (line_begin == 0) { move_cursor(0); return true; }
            const auto previous = text_.rfind('\n',line_begin-2);
            target_begin = previous == std::string::npos ? 0 : previous+1;
            target_end = line_begin-1;
        }
        else
        {
            if (line_end == text_.size()) { move_cursor(text_.size()); return true; }
            target_begin = line_end+1;
            target_end = text_.find('\n',target_begin);
            if (target_end == std::string::npos) target_end = text_.size();
        }
        std::size_t target = target_begin;
        int column = 0;
        while (target < target_end)
        {
            const auto decoded = decodeUtf8(text_,target);
            if (!decoded.bytes || column + codePointWidth(decoded.code_point) > text_area_preferred_column_) break;
            column += codePointWidth(decoded.code_point);
            target += decoded.bytes;
        }
        move_cursor(target);
        return true;
    }
    if (event.key != Key::Up && event.key != Key::Down) text_area_preferred_column_ = -1;
    if (multiline && event.key == Key::Enter)
    {
        if (read_only_) return true;
        (void)remove_selection();
        text_.insert(cursor_byte_,1,'\n');
        ++cursor_byte_;
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
        const auto start = text_.rfind('\n',cursor_byte_ == 0 ? 0 : cursor_byte_-1);
        const auto end = text_.find('\n',cursor_byte_);
        move_cursor(event.key == Key::Home ? (start == std::string::npos ? 0 : start+1) :
            (end == std::string::npos ? text_.size() : end));
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
            (void)remove_selection();
            text_.insert(cursor_byte_,sanitized);
            cursor_byte_ += sanitized.size();
            if (on_change_) on_change_(text_);
            return true;
        }
        case Key::Left: move_cursor(detail::previousBoundary(text_,cursor_byte_)); return true;
        case Key::Right: move_cursor(detail::nextBoundary(text_,cursor_byte_)); return true;
        case Key::Home: move_cursor(0); return true;
        case Key::End: move_cursor(text_.size()); return true;
        case Key::Backspace:
        {
            if (read_only_) return true;
            if (!remove_selection())
            {
                if (cursor_byte_ == 0) return true;
                const auto previous = detail::previousBoundary(text_,cursor_byte_);
                text_.erase(previous,cursor_byte_-previous);
                cursor_byte_ = previous;
            }
            if (on_change_) on_change_(text_);
            return true;
        }
        case Key::Delete:
        {
            if (read_only_) return true;
            if (!remove_selection())
            {
                if (cursor_byte_ == text_.size()) return true;
                text_.erase(cursor_byte_,detail::nextBoundary(text_,cursor_byte_)-cursor_byte_);
            }
            if (on_change_) on_change_(text_);
            return true;
        }
        case Key::Character:
        {
            if (event.character < 32 || event.character == 127 || event.ctrl || event.alt) return false;
            if (read_only_) return true;
            const auto encoded = encodeUtf8(event.character);
            (void)remove_selection();
            text_.insert(cursor_byte_,encoded);
            cursor_byte_ += encoded.size();
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
        const auto decoded = decodeUtf8(text_,cursor_byte_);
        if (!decoded.bytes || columns + (masked ? 1 : codePointWidth(decoded.code_point)) > desired) break;
        columns += masked ? 1 : codePointWidth(decoded.code_point);
        cursor_byte_ += decoded.bytes;
    }
}
