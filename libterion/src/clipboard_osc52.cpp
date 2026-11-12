/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */
#include <terion/terminal.h>
#include <string>

namespace terion {
void AnsiTerminalBackend::copyToSystemClipboard(std::string_view text)
{
    // Keep OSC escape payload bounded. A terminal may refuse OSC52 entirely.
    if (text.size() > 65536) return;
    static constexpr char alphabet[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string payload;
    payload.reserve(8 + 4 * ((text.size() + 2) / 3));
    payload = "\033]52;c;";
    for (std::size_t i = 0; i < text.size(); i += 3)
    {
        const auto a = static_cast<unsigned char>(text[i]);
        const auto b = i+1 < text.size() ? static_cast<unsigned char>(text[i+1]) : 0;
        const auto c = i+2 < text.size() ? static_cast<unsigned char>(text[i+2]) : 0;
        payload += alphabet[a >> 2];
        payload += alphabet[((a & 3) << 4) | (b >> 4)];
        payload += i+1 < text.size() ? alphabet[((b & 15) << 2) | (c >> 6)] : '=';
        payload += i+2 < text.size() ? alphabet[c & 63] : '=';
    }
    payload += "\033\\";
    write(payload);
    flush();
}
} /* End of namespace terion */
