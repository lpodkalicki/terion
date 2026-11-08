/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#ifndef TERION_TERMINAL_H
#define TERION_TERMINAL_H

#include <deque>
#include <chrono>
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
    Backspace,
    Tab,
    BackTab,
    Up,
    Down,
    Left,
    Right,
    Home,
    End,
    Delete,
    Insert,
    PageUp,
    PageDown,
    Resize,
    Mouse
}; /* End of enum Key */

enum class MouseButton
{
    None,
    Left,
    Middle,
    Right
}; /* End of enum MouseButton */

enum class MouseAction
{
    Press,
    Release,
    Move,
    Scroll
}; /* End of enum MouseAction */

enum class MousePointerShape
{
    Default,
    Text,
    Pointer,
    Grab,
    Grabbing
}; /* End of enum MousePointerShape */

struct TerminalCapabilities
{
    bool trueColor = true;
    bool unicode = true;
    bool mouse = true;
    bool mousePointerShape = false;
}; /* End of struct TerminalCapabilities */

struct MouseEvent
{
    Point position{};
    MouseButton button = MouseButton::None;
    MouseAction action = MouseAction::Move;
    int scrollX = 0;
    int scrollY = 0;
    bool shift = false;
    bool alt = false;
    bool ctrl = false;
}; /* End of struct MouseEvent */

struct Event
{
    Key key = Key::Unknown;
    char32_t character = 0;
    Size size{};
    MouseEvent mouse{};
    bool shift = false;
    bool alt = false;
    bool ctrl = false;
}; /* End of struct Event */

struct Shortcut
{
    Key key = Key::Unknown;
    char32_t character = 0;
    bool shift = false;
    bool alt = false;
    bool ctrl = false;

    [[nodiscard]] bool matches(const Event& event) const noexcept
    {
        return event.key == key && (key != Key::Character || event.character == character) &&
            event.shift == shift && event.alt == alt && event.ctrl == ctrl;
    }
}; /* End of struct Shortcut */

// Stateful incremental decoder. Incomplete UTF-8 and escape sequences are buffered.
// Call nextEvent(true) after an input timeout to resolve incomplete sequences.
class InputDecoder
{
public:
    void feed(std::string_view bytes);
    [[nodiscard]] std::optional<Event> nextEvent(bool flush_escape = false);

private:
    std::deque<unsigned char> bytes_;
}; /* End of class InputDecoder */

class TerminalBackend
{
public:
    virtual ~TerminalBackend() = default;

    [[nodiscard]] virtual Size size() const = 0;
    virtual void write(std::string_view bytes) = 0;
    virtual void flush() = 0;
    virtual std::optional<Event> pollEvent() = 0;
    [[nodiscard]] virtual TerminalCapabilities capabilities() const { return {}; }

    virtual void setMousePointerShape(MousePointerShape shape)
    {
        std::string_view name = "default";
        switch (shape)
        {
            case MousePointerShape::Text: name = "text"; break;
            case MousePointerShape::Pointer: name = "pointer"; break;
            case MousePointerShape::Grab: name = "grab"; break;
            case MousePointerShape::Grabbing: name = "grabbing"; break;
            default: break;
        }
        write(std::string("\033]22;") + std::string(name) + "\033\\");
        flush();
    }
}; /* End of class TerminalBackend */

class AnsiTerminalBackend final : public TerminalBackend
{
public:
    [[nodiscard]] static TerminalCapabilities detectCapabilities(std::string_view terminal_program,
        std::string_view pointer_override = {});

    AnsiTerminalBackend();
    ~AnsiTerminalBackend() override;

    AnsiTerminalBackend(const AnsiTerminalBackend&) = delete;
    AnsiTerminalBackend& operator=(const AnsiTerminalBackend&) = delete;

    [[nodiscard]] Size size() const override;
    [[nodiscard]] TerminalCapabilities capabilities() const override { return capabilities_; }
    void write(std::string_view bytes) override;
    void flush() override;
    std::optional<Event> pollEvent() override;
    void setMousePointerShape(MousePointerShape shape) override;

private:
    termios previous_{};
    bool active_ = false;
    Size last_size_{};
    InputDecoder decoder_;
    TerminalCapabilities capabilities_{};
    std::chrono::steady_clock::time_point last_input_{};
}; /* End of class AnsiTerminalBackend */

class HeadlessTerminalBackend final : public TerminalBackend
{
public:
    explicit HeadlessTerminalBackend(const Size size) : size_(size) {}

    [[nodiscard]] Size size() const override { return size_; }
    [[nodiscard]] TerminalCapabilities capabilities() const override
    {
        return {.mousePointerShape = true};
    }
    void resize(const Size size)
    {
        if (size_ == size) return;
        size_ = size;
        events_.push_back(Event{.key = Key::Resize,.size = size});
    }
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
