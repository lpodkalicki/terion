/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

// Measures terminal cursor advancement instead of guessing Unicode width.
// Run inside Terminal.app, iTerm2 or VS Code's integrated terminal.
#include <array>
#include <cerrno>
#include <chrono>
#include <cstring>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <poll.h>
#include <termios.h>
#include <unistd.h>
#include <terion/grapheme.h>
#include <terion/emoji_compat.h>

namespace {

bool send(std::string_view bytes)
{
    while (!bytes.empty())
    {
        const ssize_t count = ::write(STDOUT_FILENO,bytes.data(),bytes.size());
        if (count < 0 && errno == EINTR) continue;
        if (count <= 0) return false;
        bytes.remove_prefix(static_cast<std::size_t>(count));
    }
    return true;
}

class TerminalSession
{
public:
    TerminalSession()
    {
        if (!::isatty(STDIN_FILENO) || !::isatty(STDOUT_FILENO)) return;
        if (::tcgetattr(STDIN_FILENO,&original_) != 0) return;
        termios raw = original_;
        ::cfmakeraw(&raw);
        if (::tcsetattr(STDIN_FILENO,TCSAFLUSH,&raw) != 0) return;
        active_ = true;
        (void)send("\033[?1049h\033[?25l\033[2J\033[H");
    }

    ~TerminalSession()
    {
        if (!active_) return;
        (void)send("\033[?25h\033[?1049l");
        (void)::tcsetattr(STDIN_FILENO,TCSAFLUSH,&original_);
    }

    TerminalSession(const TerminalSession&) = delete;
    TerminalSession& operator=(const TerminalSession&) = delete;
    [[nodiscard]] bool active() const noexcept { return active_; }

private:
    termios original_{};
    bool active_ = false;
};

// Parse the standard ANSI Cursor Position Report: ESC [ row ; column R.
std::optional<int> responseColumn(const std::string_view data, const int expected_row)
{
    for (std::size_t i = 0; i+2 < data.size(); ++i)
    {
        if (data[i] != '\033' || data[i+1] != '[') continue;
        std::size_t j = i+2;
        auto decimal = [&](int& value) {
            value = 0;
            const auto start = j;
            while (j < data.size() && data[j] >= '0' && data[j] <= '9')
            {
                if (value > 100000) return false;
                value = value*10 + data[j++]-'0';
            }
            return j > start;
        };
        int row = 0;
        int column = 0;
        if (!decimal(row) || j >= data.size() || data[j++] != ';' ||
            !decimal(column) || j >= data.size() || data[j] != 'R') continue;
        if (row == expected_row) return column;
    }
    return std::nullopt;
}

std::optional<int> measure(std::string_view value)
{
    constexpr int probe_row = 2;
    if (!send("\033[2;1H\033[2K") || !send(value) || !send("\033[6n")) return std::nullopt;
    std::string data;
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(350);
    while (std::chrono::steady_clock::now() < deadline)
    {
        const auto remain = std::chrono::duration_cast<std::chrono::milliseconds>(
            deadline-std::chrono::steady_clock::now()).count();
        pollfd descriptor{.fd=STDIN_FILENO,.events=POLLIN,.revents=0};
        const int ready = ::poll(&descriptor,1,static_cast<int>(remain));
        if (ready < 0 && errno == EINTR) continue;
        if (ready <= 0) break;
        if (!(descriptor.revents & POLLIN)) continue;
        std::array<char,128> bytes{};
        const auto count = ::read(STDIN_FILENO,bytes.data(),bytes.size());
        if (count <= 0) break;
        data.append(bytes.data(),static_cast<std::size_t>(count));
        if (const auto column = responseColumn(data,probe_row)) return *column-1;
        if (data.size() > 1024) break;
    }
    return std::nullopt;
}

} // namespace

int main()
{
    constexpr std::array<std::string_view,10> samples{
        "A", "│", "·", "e\u0301", "界", "🇵🇱", "👨‍👩‍👧", "👍🏽", "❤️", "1️⃣"
    };
    std::array<std::optional<int>,samples.size()> observed{};
    std::array<std::optional<int>,samples.size()> compatible{};
    {
        TerminalSession terminal;
        if (!terminal.active())
        {
            std::cerr << "Run unicode_width_probe directly in an interactive terminal.\n";
            return 1;
        }
        for (std::size_t i = 0; i < samples.size(); ++i)
        {
            observed[i] = measure(samples[i]);
            const auto normalized = terion::compatibleTerminalGrapheme(samples[i],terion::displayWidth(samples[i]));
            compatible[i] = measure(normalized);
        }
    }
    std::cout << "Terion Unicode width diagnostic (cursor-position report)\n"
              << "Glyph            Model   Terminal  Result\n";
    bool mismatch = false;
    bool unsupported = false;
    for (std::size_t i = 0; i < samples.size(); ++i)
    {
        const int expected = terion::displayWidth(samples[i]);
        const auto actual = observed[i];
        const bool differs = actual && *actual != expected;
        mismatch |= differs;
        unsupported |= !actual.has_value();
        std::cout << std::string(samples[i]) << "\t\t" << expected << "\t"
                  << (actual ? std::to_string(*actual) : "no CPR") << "\t"
                  << (!actual ? "unsupported" : differs ? "MISMATCH" : "OK") << '\n';
    }
    std::cout << "\nCompatible glyph output (terminal columns vs model):\n";
    for (std::size_t i = 0; i < samples.size(); ++i)
    {
        if (terion::compatibleTerminalGrapheme(samples[i],terion::displayWidth(samples[i])) == samples[i])
            continue;
        const auto target = terion::displayWidth(samples[i]);
        std::cout << samples[i] << "\t" << target << "\t" <<
            (compatible[i] ? std::to_string(*compatible[i]) : "no CPR") << "\t" <<
            (compatible[i] && *compatible[i] == target ? "OK" : "MISMATCH") << '\n';
    }
    if (mismatch)
        std::cout << "Width mismatch detected. Run Terion with TERION_EMOJI_COMPAT=1.\n";
    else if (unsupported)
        std::cout << "Some cursor reports were unavailable; the result is inconclusive.\n";
    else
        std::cout << "All sampled grapheme widths match (other Unicode may differ).\n";
    return mismatch ? 2 : unsupported ? 3 : 0;
}
