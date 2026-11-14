/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <terion/terminal.h>
#include <terion/utf8.h>
#include <limits>
#include <algorithm>
#include <array>

using namespace terion;

void InputDecoder::feed(const std::string_view bytes)
{
    for (const unsigned char c : bytes) bytes_.push_back(c);
}

std::optional<Event> InputDecoder::nextEvent(const bool flush_escape)
{
    if (pasting_)
    {
        constexpr std::string_view end_marker = "\033[201~";
        while (!bytes_.empty())
        {
            const auto marker = std::search(bytes_.begin(),bytes_.end(),end_marker.begin(),end_marker.end());
            if (marker != bytes_.end())
            {
                const auto count = static_cast<std::size_t>(std::distance(bytes_.begin(),marker));
                for (std::size_t i = 0; i < count; ++i)
                {
                    if (paste_buffer_.size() < 1024 * 1024)
                        paste_buffer_ += static_cast<char>(bytes_.front());
                    bytes_.pop_front();
                }
                for (std::size_t i = 0; i < end_marker.size(); ++i) bytes_.pop_front();
                pasting_ = false;
                // Preserve multiline paste. Individual editors sanitize their own input.
                Event event{.key = Key::Paste,.text = std::move(paste_buffer_)};
                paste_buffer_.clear();
                return event;
            }
            // Keep a possible split terminator until the next feed().
            std::size_t keep = 0;
            for (std::size_t len = 1; len < end_marker.size() && len <= bytes_.size(); ++len)
                if (std::equal(bytes_.end()-static_cast<std::ptrdiff_t>(len),bytes_.end(),
                    end_marker.begin())) keep = len;
            while (bytes_.size() > keep)
            {
                if (paste_buffer_.size() < 1024 * 1024)
                    paste_buffer_ += static_cast<char>(bytes_.front());
                bytes_.pop_front();
            }
            return std::nullopt;
        }
        return std::nullopt;
    }
    if (bytes_.empty()) return std::nullopt;

    const auto first = bytes_.front();
    if (first == 27)
    {
        if (bytes_.size() == 1 && !flush_escape) return std::nullopt;

        if (bytes_.size() >= 2 && (bytes_[1] == 127 || bytes_[1] == 8))
        {
            bytes_.pop_front();
            bytes_.pop_front();
            return Event{.key = Key::Backspace,.alt = true};
        }

        if (bytes_.size() >= 2 && (bytes_[1] == '[' || bytes_[1] == 'O'))
        {
            std::size_t end = 2;
            while (end < bytes_.size() && end < 64 && (bytes_[end] < 0x40 || bytes_[end] > 0x7E)) ++end;
            if (end >= bytes_.size() && !flush_escape && end < 64) return std::nullopt;
            if (end < bytes_.size() && end < 64)
            {
                const auto final = bytes_[end];
                // Numeric CSI parameters are also used by CSI-u and modifyOtherKeys.
                std::array<int,3> parameters{};
                std::size_t parameter_count = 0;
                bool numeric = bytes_[1] == '[';
                bool has_digit = false;
                for (std::size_t i = 2; numeric && i < end; ++i)
                {
                    const auto c = bytes_[i];
                    if (c >= '0' && c <= '9' &&
                        parameters[parameter_count] <= (std::numeric_limits<int>::max() - (c-'0')) / 10)
                    {
                        parameters[parameter_count] = parameters[parameter_count] * 10 + c-'0';
                        has_digit = true;
                    }
                    else if (c == ';' && has_digit && parameter_count + 1 < parameters.size())
                    {
                        ++parameter_count;
                        has_digit = false;
                    }
                    else numeric = false;
                }
                numeric = numeric && has_digit;
                if (numeric) ++parameter_count;
                const auto modifier_flags = [](const int modifier) {
                    return modifier >= 1 && modifier <= 8 ? modifier-1 : -1;
                };
                if (numeric && ((final == 'u' && parameter_count <= 2) ||
                    (final == '~' && parameter_count == 3 && parameters[0] == 27)))
                {
                    const int value = final == 'u' ? parameters[0] : parameters[2];
                    const int modifier = parameter_count >= 2 ? parameters[1] : 1;
                    const int flags = modifier_flags(modifier);
                    Key key = Key::Character;
                    char32_t character = static_cast<char32_t>(value);
                    if (value == 9) key = (flags & 1) ? Key::BackTab : Key::Tab;
                    else if (value == 13 || value == 10) key = Key::Enter;
                    else if (value == 27) key = Key::Escape;
                    else if (value == 8 || value == 127) key = Key::Backspace;
                    else if (value < 32 || value > 0x10FFFF ||
                        (value >= 0xD800 && value <= 0xDFFF)) key = Key::Unknown;
                    if (key == Key::Character && (flags & 4) && character >= U'A' && character <= U'Z')
                        character += 32;
                    for (std::size_t i = 0; i <= end; ++i) bytes_.pop_front();
                    if (flags < 0) key = Key::Unknown;
                    return Event{.key = key,.character = key == Key::Character ? character : 0,
                        .shift = (flags & 1) != 0,.alt = (flags & 2) != 0,
                        .ctrl = (flags & 4) != 0};
                }
                if (bytes_.size() > 3 && bytes_[2] == '<' && (final == 'M' || final == 'm'))
                {
                    int fields[3] = {};
                    int field = 0;
                    bool valid = true;
                    bool has_digit = false;
                    for (std::size_t i = 3; i < end; ++i)
                    {
                        const auto c = bytes_[i];
                        if (c == ';' && has_digit && field < 2)
                        {
                            ++field;
                            has_digit = false;
                        }
                        else if (c >= '0' && c <= '9' &&
                            fields[field] <= (std::numeric_limits<int>::max() - (c - '0')) / 10)
                        {
                            fields[field] = fields[field] * 10 + c - '0';
                            has_digit = true;
                        }
                        else { valid = false; break; }
                    }
                    valid = valid && field == 2 && has_digit && fields[1] > 0 && fields[2] > 0;
                    for (std::size_t i = 0; i <= end; ++i) bytes_.pop_front();
                    if (!valid) return Event{.key = Key::Unknown};

                    const int code = fields[0];
                    const int button = code & 3;
                    MouseEvent mouse {
                        .position = {.x = fields[1] - 1,.y = fields[2] - 1},
                        .button = button == 0 ? MouseButton::Left : button == 1 ? MouseButton::Middle :
                            button == 2 ? MouseButton::Right : MouseButton::None,
                        .action = (code & 64) ? MouseAction::Scroll : final == 'm' ? MouseAction::Release :
                            (code & 32) ? MouseAction::Move : MouseAction::Press,
                        .shift = (code & 4) != 0,
                        .alt = (code & 8) != 0,
                        .ctrl = (code & 16) != 0
                    };
                    if (mouse.action == MouseAction::Scroll)
                    {
                        mouse.button = MouseButton::None;
                        if (button < 2) mouse.scroll_y = button == 0 ? 1 : -1;
                        else mouse.scroll_x = button == 2 ? -1 : 1;
                    }
                    return Event{.key = Key::Mouse,.mouse = mouse};
                }
                Key key = Key::Unknown;
                switch (final)
                {
                    case 'A': key = Key::Up; break;
                    case 'B': key = Key::Down; break;
                    case 'C': key = Key::Right; break;
                    case 'D': key = Key::Left; break;
                    case 'H': key = Key::Home; break;
                    case 'F': key = Key::End; break;
                    case 'P': if (bytes_[1] == 'O' || bytes_[1] == '[') key = Key::F1; break;
                    case 'Q': if (bytes_[1] == 'O' || bytes_[1] == '[') key = Key::F2; break;
                    case 'R': if (bytes_[1] == 'O' || bytes_[1] == '[') key = Key::F3; break;
                    case 'S': if (bytes_[1] == 'O' || bytes_[1] == '[') key = Key::F4; break;
                    case 'Z': if (bytes_[1] == '[') key = Key::BackTab; break;
                    case '~':
                    {
                        int number = 0;
                        for (std::size_t i = 2; i < end && bytes_[i] >= '0' && bytes_[i] <= '9'; ++i)
                            number = number * 10 + bytes_[i] - '0';
                        switch (number)
                        {
                            case 200:
                                for (std::size_t i = 0; i <= end; ++i) bytes_.pop_front();
                                paste_buffer_.clear();
                                pasting_ = true;
                                return nextEvent(flush_escape);
                            case 1: case 7: key = Key::Home; break;
                            case 2: key = Key::Insert; break;
                            case 3: key = Key::Delete; break;
                            case 4: case 8: key = Key::End; break;
                            case 5: key = Key::PageUp; break;
                            case 6: key = Key::PageDown; break;
                            case 11: key = Key::F1; break;
                            case 12: key = Key::F2; break;
                            case 13: key = Key::F3; break;
                            case 14: key = Key::F4; break;
                            case 15: key = Key::F5; break;
                            case 17: key = Key::F6; break;
                            case 18: key = Key::F7; break;
                            case 19: key = Key::F8; break;
                            case 20: key = Key::F9; break;
                            case 21: key = Key::F10; break;
                            case 23: key = Key::F11; break;
                            case 24: key = Key::F12; break;
                            default: break;
                        }
                        break;
                    }
                    default: break;
                }
                bool shift = false;
                bool alt = false;
                bool ctrl = false;
                if (bytes_[1] == '[')
                {
                    int modifier = 0;
                    bool after_separator = false;
                    for (std::size_t i = 2; i < end; ++i)
                    {
                        if (bytes_[i] == ';') { after_separator = true; modifier = 0; }
                        else if (after_separator && bytes_[i] >= '0' && bytes_[i] <= '9')
                            modifier = modifier < 10 ? modifier * 10 + bytes_[i] - '0' : 99;
                    }
                    if (after_separator && modifier >= 2 && modifier <= 8)
                    {
                        shift = ((modifier-1) & 1) != 0;
                        alt = ((modifier-1) & 2) != 0;
                        ctrl = ((modifier-1) & 4) != 0;
                    }
                }
                for (std::size_t i = 0; i <= end; ++i) bytes_.pop_front();
                return Event{.key = key,.shift = shift,.alt = alt,.ctrl = ctrl};
            }
        }

        if (bytes_.size() >= 2 && bytes_[1] >= 32 && bytes_[1] < 127)
        {
            const char32_t character = bytes_[1];
            bytes_.pop_front();
            bytes_.pop_front();
            return Event{.key = Key::Character,.character = character,.alt = true};
        }

        // Option/Alt+Unicode is commonly encoded as ESC followed by UTF-8.
        if (bytes_.size() >= 2 && bytes_[1] >= 0xC2 && bytes_[1] <= 0xF4)
        {
            const auto lead = bytes_[1];
            const std::size_t length = lead <= 0xDF ? 2 : lead <= 0xEF ? 3 : 4;
            if (bytes_.size() < length+1 && !flush_escape) return std::nullopt;
            if (bytes_.size() >= length+1)
            {
                std::string encoded;
                for (std::size_t i = 1; i <= length; ++i)
                    encoded += static_cast<char>(bytes_[i]);
                const auto decoded = decodeUtf8(encoded,0);
                if (decoded.bytes == length && decoded.code_point != 0xFFFD)
                {
                    for (std::size_t i = 0; i <= length; ++i) bytes_.pop_front();
                    return Event{.key = Key::Character,.character = decoded.code_point,.alt = true};
                }
            }
        }

        bytes_.pop_front();
        return Event{.key = Key::Escape};
    }

    if (first == 13 || first == 10 || first == 8 || first == 127 || first == 9)
    {
        bytes_.pop_front();
        return Event{.key = first == 13 || first == 10 ? Key::Enter :
            first == 9 ? Key::Tab : Key::Backspace};
    }
    if (first < 32)
    {
        bytes_.pop_front();
        if (first >= 1 && first <= 26)
            return Event{.key = Key::Character,.character = U'a'+first-1,.ctrl = true};
        if (first == 0) return Event{.key = Key::Character,.character = U' ',.ctrl = true};
        return Event{.key = Key::Unknown};
    }

    const std::size_t length = first < 0x80 ? 1 : first >= 0xC2 && first <= 0xDF ? 2 :
        first >= 0xE0 && first <= 0xEF ? 3 : first >= 0xF0 && first <= 0xF4 ? 4 : 1;
    if (bytes_.size() < length)
    {
        if (!flush_escape) return std::nullopt;
        bytes_.pop_front();
        return Event{.key = Key::Character,.character = 0xFFFD};
    }

    std::string encoded;
    for (std::size_t i = 0; i < length; ++i) encoded += static_cast<char>(bytes_[i]);
    const auto decoded = decodeUtf8(encoded,0);
    for (std::size_t i = 0; i < decoded.bytes; ++i) bytes_.pop_front();
    return Event{.key = Key::Character,.character = decoded.code_point};
}
