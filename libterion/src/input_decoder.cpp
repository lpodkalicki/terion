/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <terion/terminal.h>
#include <terion/utf8.h>
#include <limits>

using namespace terion;

void InputDecoder::feed(const std::string_view bytes)
{
    for (const unsigned char c : bytes) bytes_.push_back(c);
}

std::optional<Event> InputDecoder::nextEvent(const bool flush_escape)
{
    if (bytes_.empty()) return std::nullopt;

    const auto first = bytes_.front();
    if (first == 27)
    {
        if (bytes_.size() == 1 && !flush_escape) return std::nullopt;

        if (bytes_.size() >= 2 && (bytes_[1] == '[' || bytes_[1] == 'O'))
        {
            std::size_t end = 2;
            while (end < bytes_.size() && end < 64 && (bytes_[end] < 0x40 || bytes_[end] > 0x7E)) ++end;
            if (end >= bytes_.size() && !flush_escape && end < 64) return std::nullopt;
            if (end < bytes_.size() && end < 64)
            {
                const auto final = bytes_[end];
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
                        if (button < 2) mouse.scrollY = button == 0 ? 1 : -1;
                        else mouse.scrollX = button == 2 ? -1 : 1;
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
                    case 'Z':
                        if (bytes_[1] == '[' && end == 2) key = Key::BackTab;
                        break;
                    case '~':
                    {
                        int number = 0;
                        for (std::size_t i = 2; i < end && bytes_[i] >= '0' && bytes_[i] <= '9'; ++i)
                            number = number * 10 + bytes_[i] - '0';
                        switch (number)
                        {
                            case 1: case 7: key = Key::Home; break;
                            case 2: key = Key::Insert; break;
                            case 3: key = Key::Delete; break;
                            case 4: case 8: key = Key::End; break;
                            case 5: key = Key::PageUp; break;
                            case 6: key = Key::PageDown; break;
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
