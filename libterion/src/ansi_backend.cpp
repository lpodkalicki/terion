/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <sys/ioctl.h>
#include <poll.h>
#include <unistd.h>
#include <cerrno>
#include <chrono>
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
    last_size_ = size();
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
    const auto current_size = size();
    if (current_size != last_size_)
    {
        last_size_ = current_size;
        return Event{.key = Key::Resize,.size = current_size};
    }

    pollfd descriptor{.fd = STDIN_FILENO, .events = POLLIN,.revents = 0};
    if (::poll(&descriptor,1,0) > 0 && (descriptor.revents & POLLIN))
    {
        char data[128];
        const auto n = ::read(STDIN_FILENO,data,sizeof(data));
        if (n > 0)
        {
            decoder_.feed(std::string_view(data,static_cast<std::size_t>(n)));
            last_input_ = std::chrono::steady_clock::now();
        }
    }

    const bool timed_out = std::chrono::steady_clock::now() - last_input_ >= std::chrono::milliseconds(35);
    return decoder_.nextEvent(timed_out);
}
