/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <sys/ioctl.h>
#include <poll.h>
#include <unistd.h>
#include <cerrno>
#include <chrono>
#include <algorithm>
#include <cstdint>
#include <utility>
#include <cstdlib>
#include <stdexcept>
#include <terion/terminal.h>

using namespace terion;

TerminalCapabilities AnsiTerminalBackend::detectCapabilities(const std::string_view terminal_program,
    const std::string_view pointer_override)
{
    TerminalCapabilities capabilities;
    capabilities.mouse_pointer_shape = terminal_program == "kitty" || terminal_program == "ghostty";
    if (pointer_override == "1") capabilities.mouse_pointer_shape = true;
    if (pointer_override == "0") capabilities.mouse_pointer_shape = false;
    return capabilities;
}

// XTGETTCAP: DCS (1|0)+r HEX_NAME[=HEX_VALUE] ST
// DECRQM: CSI ? MODE ; STATUS $ y
void TerminalCapabilityProbe::process(const std::string_view sequence)
{
    if (sequence.starts_with("\033P") && sequence.ends_with("\033\\") && sequence.size() >= 7 &&
        (sequence.substr(2,3) == "1+r" || sequence.substr(2,3) == "0+r"))
    {
        const bool supported = sequence[2] == '1';
        const auto payload = sequence.substr(5,sequence.size()-7);
        const auto name = payload.substr(0,payload.find('='));
        if (name == "5463" || name == "524742") // Tc and RGB
        {
            capabilities_.actively_probed = true;
            if (supported) capabilities_.true_color = true;
            else if (++rejected_colors_ >= 2) capabilities_.true_color = false;
        }
    }
    else if (sequence.starts_with("\033[?") && sequence.ends_with("$y"))
    {
        const auto body = sequence.substr(3,sequence.size()-5);
        const auto separator = body.find(';');
        if (separator == std::string_view::npos) return;
        const auto mode = body.substr(0,separator);
        const auto status = body.substr(separator+1);
        if (mode == "1006" || mode == "1003")
        {
            if (status == "0") capabilities_.mouse = false;
            if (status == "0" || status == "1" || status == "2" || status == "3" || status == "4")
                capabilities_.actively_probed = true;
        }
    }
}

std::string TerminalCapabilityProbe::feed(const std::string_view bytes)
{
    std::string ordinary;
    pending_.append(bytes);
    while (!pending_.empty())
    {
        if (pending_[0] != '\033')
        {
            ordinary += pending_[0];
            pending_.erase(0,1);
            continue;
        }
        if (pending_.size() == 1) break;
        const bool dcs = pending_[1] == 'P';
        const bool csi = pending_[1] == '[';
        if (!dcs && !csi)
        {
            ordinary += pending_[0];
            pending_.erase(0,1);
            continue;
        }
        if (dcs)
        {
            if (pending_.size() < 5)
            {
                if (std::string_view("\033P1+r").starts_with(pending_) ||
                    std::string_view("\033P0+r").starts_with(pending_)) break;
                ordinary += pending_[0];
                pending_.erase(0,1);
                continue;
            }
            if (pending_.substr(2,3) != "1+r" && pending_.substr(2,3) != "0+r")
            {
                ordinary += pending_[0];
                pending_.erase(0,1);
                continue;
            }
            const auto end = pending_.find("\033\\",5);
            if (end == std::string::npos)
            {
                if (pending_.size() <= 128) break;
            }
            else
            {
                const std::string_view candidate(pending_.data(),end+2);
                process(candidate);
                pending_.erase(0,end+2);
                continue;
            }
        }
        else
        {
            if (pending_.size() < 3)
            {
                if (std::string_view("\033[?").starts_with(pending_)) break;
            }
            else if (pending_[2] == '?')
            {
                // Only buffer DECRQM responses for the modes we queried.
                const auto final = pending_.find_first_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz~",3);
                if (final == std::string::npos)
                {
                    if (pending_.size() <= 48) break;
                }
                else
                {
                    const std::string_view candidate(pending_.data(),final+1);
                    if (candidate.ends_with("$y") &&
                        (candidate.starts_with("\033[?1006;") || candidate.starts_with("\033[?1003;")))
                    {
                        process(candidate);
                        pending_.erase(0,final+1);
                        continue;
                    }
                }
            }
        }
        ordinary += pending_[0];
        pending_.erase(0,1);
    }
    return ordinary;
}

std::string TerminalCapabilityProbe::finish()
{
    return std::exchange(pending_,std::string{});
}

void AnsiTerminalBackend::probeCapabilities()
{
    TerminalCapabilityProbe probe(capabilities_);
    write("\033P+q5463;524742\033\\\033[?1006$p\033[?1003$p");
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(85);
    while (std::chrono::steady_clock::now() < deadline)
    {
        const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
            deadline - std::chrono::steady_clock::now()).count();
        pollfd descriptor{.fd = STDIN_FILENO,.events = POLLIN,.revents = 0};
        const auto ready = ::poll(&descriptor,1,static_cast<int>(std::max<std::int64_t>(0,remaining)));
        if (ready < 0 && errno == EINTR) continue;
        if (ready <= 0 || !(descriptor.revents & POLLIN)) break;
        char buffer[256];
        const auto count = ::read(STDIN_FILENO,buffer,sizeof(buffer));
        if (count <= 0) break;
        decoder_.feed(probe.feed(std::string_view(buffer,static_cast<std::size_t>(count))));
    }
    decoder_.feed(probe.finish());
    capabilities_ = probe.capabilities();
}

void AnsiTerminalBackend::setMousePointerShape(const MousePointerShape shape)
{
    if (!capabilities_.mouse_pointer_shape) return;
    TerminalBackend::setMousePointerShape(shape);
}

AnsiTerminalBackend::AnsiTerminalBackend()
{
    const char* terminal_program = std::getenv("TERM_PROGRAM");
    const char* pointer_override = std::getenv("TERION_MOUSE_POINTER_SHAPE");
    capabilities_ = detectCapabilities(terminal_program ? terminal_program : "",
        pointer_override ? pointer_override : "");
    if (!::isatty(STDIN_FILENO) || !::isatty(STDOUT_FILENO)) throw std::runtime_error("Terion requires a TTY");
    if (::tcgetattr(STDIN_FILENO,&previous_) != 0) throw std::runtime_error("tcgetattr failed");
    termios raw = previous_;
    ::cfmakeraw(&raw);
    if (::tcsetattr(STDIN_FILENO,TCSAFLUSH,&raw) != 0) throw std::runtime_error("tcsetattr failed");
    active_ = true;
    last_size_ = size();
    try
    {
        probeCapabilities();
        write("\033[?1049h\033[?25l\033[?2004h");
        if (capabilities_.mouse) write("\033[?1003h\033[?1006h");
    }
    catch (...) { ::tcsetattr(STDIN_FILENO,TCSAFLUSH,&previous_); active_ = false; throw; }
}

AnsiTerminalBackend::~AnsiTerminalBackend()
{
    if (active_)
    {
        try
        {
            if (capabilities_.mouse_pointer_shape) write("\033]22;\033\\");
            write("\033[?2004l\033[?1006l\033[?1003l\033[0m\033[?25h\033[?1049l");
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
