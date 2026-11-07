/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#ifndef TERION_TERMINAL_H
#define TERION_TERMINAL_H

#include <deque>
#include <optional>
#include <string>
#include <string_view>
#include <termios.h>

#include <terion/geometry.h>

namespace terion {

enum class Key
{
    Unknown,
    Escape,
    Character,
    Enter,
    Backspace
}; /* End of enum Key */

struct Event
{
    Key key = Key::Unknown;
    char32_t character = 0;
}; /* End of struct Event */

class TerminalBackend
{
public:
    virtual ~TerminalBackend() = default;

    [[nodiscard]] virtual Size size() const = 0;
    virtual void write(std::string_view bytes) = 0;
    virtual void flush() = 0;
    virtual std::optional<Event> pollEvent() = 0;
}; /* End of class TerminalBackend */

class AnsiTerminalBackend final : public TerminalBackend
{
public:
    AnsiTerminalBackend();
    ~AnsiTerminalBackend() override;

    AnsiTerminalBackend(const AnsiTerminalBackend&) = delete;
    AnsiTerminalBackend& operator=(const AnsiTerminalBackend&) = delete;

    [[nodiscard]] Size size() const override;
    void write(std::string_view bytes) override;
    void flush() override;
    std::optional<Event> pollEvent() override;

private:
    termios previous_{};
    bool active_ = false;
}; /* End of class AnsiTerminalBackend */

class HeadlessTerminalBackend final : public TerminalBackend
{
public:
    explicit HeadlessTerminalBackend(const Size size) : size_(size) {}

    [[nodiscard]] Size size() const override { return size_; }
    void resize(const Size size) { size_ = size; }
    void write(const std::string_view bytes) override { output_.append(bytes); }
    void flush() override { ++flush_count_; }

    std::optional<Event> pollEvent() override
    {
        if (events_.empty()) return std::nullopt;
        auto event = events_.front();
        events_.pop_front();
        return event;
    }

    void push(const Event event) { events_.push_back(event); }
    [[nodiscard]] const std::string& output() const { return output_; }
    void clearOutput() { output_.clear(); }
    [[nodiscard]] unsigned flushCount() const { return flush_count_; }

private:
    Size size_;
    std::string output_;
    std::deque<Event> events_;
    unsigned flush_count_ = 0;
}; /* End of class HeadlessTerminalBackend */

} /* End of namespace terion::terminal */

#endif /* TERION_TERMINAL_H */
