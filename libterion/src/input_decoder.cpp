/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <terion/terminal.h>
#include <terion/utf8.h>

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
                Key key = Key::Unknown;
                switch (final)
                {
                    case 'A': key = Key::Up; break;
                    case 'B': key = Key::Down; break;
                    case 'C': key = Key::Right; break;
                    case 'D': key = Key::Left; break;
                    case 'H': key = Key::Home; break;
                    case 'F': key = Key::End; break;
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
                for (std::size_t i = 0; i <= end; ++i) bytes_.pop_front();
                return Event{.key = key};
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
