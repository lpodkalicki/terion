/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <sys/ioctl.h>
#include <poll.h>
#include <unistd.h>
#include <cerrno>
#include <stdexcept>
#include <terion/terminal.h>

using namespace terion;

AnsiTerminalBackend::AnsiTerminalBackend()
{
    if (!::isatty(STDIN_FILENO) || !::isatty(STDOUT_FILENO)) throw std::runtime_error("Terion requires a TTY");
    if (::tcgetattr(STDIN_FILENO,&previous_) != 0) throw std::runtime_error("tcgetattr failed");
    termios raw = previous_;
    ::cfmakeraw(&raw);
    if (::tcsetattr(STDIN_FILENO,TCSAFLUSH,&raw) != 0) throw std::runtime_error("tcsetattr failed");
    active_ = true;
    try { write("\033[?1049h\033[?25l"); }
    catch (...) { ::tcsetattr(STDIN_FILENO,TCSAFLUSH,&previous_); active_ = false; throw; }
}

AnsiTerminalBackend::~AnsiTerminalBackend()
{
    if (active_)
    {
        try
        {
            write("\033[0m\033[?25h\033[?1049l");
        }
        catch (...)
        {}
        ::tcsetattr(STDIN_FILENO,TCSAFLUSH,&previous_);
    }
}

Size AnsiTerminalBackend::size() const
{
    winsize w{};
    if (::ioctl(STDOUT_FILENO,TIOCGWINSZ,&w) != 0) return {80,24};
    return {.width = w.ws_col,.height = w.ws_row};
}

void AnsiTerminalBackend::write(std::string_view bytes)
{
    while (!bytes.empty())
    {
        const auto n = ::write(STDOUT_FILENO,bytes.data(),bytes.size());
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) throw std::runtime_error("terminal write failed");
        bytes.remove_prefix(static_cast<std::size_t>(n));
    }
}

void AnsiTerminalBackend::flush() {}

std::optional<Event> AnsiTerminalBackend::pollEvent()
{
    pollfd descriptor{.fd = STDIN_FILENO, .events = POLLIN,.revents = 0};
    if (::poll(&descriptor,1,0) <= 0 || !(descriptor.revents & POLLIN)) return std::nullopt;
    unsigned char c{};
    const auto n = ::read(STDIN_FILENO,&c,1);
    if (n != 1) return std::nullopt;
    if (c == 27) return Event{Key::Escape};
    if (c == 13) return Event{Key::Enter};
    if (c == 127) return Event{Key::Backspace};
    return Event{.key = Key::Character,.character = c};
}
