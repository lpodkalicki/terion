/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <string>
#include <string_view>
#include <thread>

#include <terion/terminal.h>

using namespace terion;

namespace {

std::string getenvOr(const char* name, const char* fallback = "(unset)")
{
    const char* value = std::getenv(name);
    return value && *value ? value : fallback;
}

std::string keyName(const Event& event)
{
    switch (event.key)
    {
        case Key::Escape: return "Escape";
        case Key::Character:
            if (event.character >= 32 && event.character <= 126)
                return std::string("Character '") + static_cast<char>(event.character) + "'";
            return "Unicode character";
        case Key::Enter: return "Enter";
        case Key::Backspace: return "Backspace";
        case Key::Tab: return "Tab";
        case Key::BackTab: return "Shift+Tab";
        case Key::Up: return "Up";
        case Key::Down: return "Down";
        case Key::Left: return "Left";
        case Key::Right: return "Right";
        case Key::Home: return "Home";
        case Key::End: return "End";
        case Key::Delete: return "Delete";
        case Key::Insert: return "Insert";
        case Key::PageUp: return "PageUp";
        case Key::PageDown: return "PageDown";
        case Key::Resize: return "Resize";
        case Key::Mouse: return "Mouse";
        case Key::Paste: return "Paste";
        default: return "Unknown";
    }
}

std::string mouseName(const Event& event)
{
    const MouseEvent& mouse = event.mouse;
    std::string action;
    switch (mouse.action)
    {
        case MouseAction::Press: action = "press"; break;
        case MouseAction::Release: action = "release"; break;
        case MouseAction::Move: action = "move"; break;
        case MouseAction::Scroll: action = "wheel"; break;
    }
    return action + " (" + std::to_string(mouse.position.x) + "," +
        std::to_string(mouse.position.y) + ") wheel=" + std::to_string(mouse.scroll_y);
}

std::string yesNo(bool value)
{
    return value ? "yes" : "no";
}

void draw(AnsiTerminalBackend& backend, const TerminalCapabilities& caps,
    const std::string& last_key, const std::string& last_mouse, std::size_t paste_bytes,
    bool seen_mouse, bool seen_paste, bool seen_resize)
{
    const Size size = backend.size();
    std::string output = "\033[H\033[2J";
    int line = 0;
    const auto print = [&](std::string_view message) {
        if (++line > size.height) return;
        // Clear the entire line; terminal clips text if window is too narrow.
        output += "\033[" + std::to_string(line) + ";1H\033[2K";
        output += message;
    };

    print("\033[1mTerion | Terminal compatibility check\033[0m   [Esc/q] exit");
    print("TERM=" + getenvOr("TERM") + "  TERM_PROGRAM=" + getenvOr("TERM_PROGRAM"));
    print("COLORTERM=" + getenvOr("COLORTERM") + "  LANG=" + getenvOr("LANG"));
    print("Size: " + std::to_string(size.width) + "x" + std::to_string(size.height));
    print("Probe replied: " + yesNo(caps.actively_probed) + " (no means fallback values)");
    print("Reported: RGB=" + yesNo(caps.true_color) + " Unicode=" + yesNo(caps.unicode) +
        " mouse=" + yesNo(caps.mouse) + " pointer-shape=" + yesNo(caps.mouse_pointer_shape));
    print("\033[2mNote: booleans may be fallback guesses, not confirmed support.\033[0m");
    print("");
    print("Visual check (look for distinct colors, aligned symbols):");
    print(" ANSI16: \033[31mRED\033[0m \033[32mGREEN\033[0m \033[34mBLUE\033[0m");
    print(" ANSI256: \033[38;5;208mORANGE\033[0m \033[38;5;45mCYAN\033[0m \033[38;5;129mPURPLE\033[0m");
    print(" Truecolor: \033[38;2;255;95;85mCORAL\033[0m \033[38;2;90;205;155mMINT\033[0m");
    print(" Unicode: Lodz: Łódź | box: ┌─┐ │ └─┘ | wide: 界 | emoji: 🚀");
    print("");
    print("Input check: press arrows, Tab, Shift+Tab; move/click/scroll mouse;");
    print(" paste some text; resize the terminal. (Pasted content is NOT displayed.)");
    print(" Keyboard: " + last_key);
    print(" Mouse: " + last_mouse + (seen_mouse ? " [seen]" : " [not seen]"));
    print(" Paste: " + std::to_string(paste_bytes) + " bytes" +
        (seen_paste ? " [seen]" : " [not seen]"));
    print(" Resize event: " + std::string(seen_resize ? "[seen]" : "[not seen]"));
    print("");
    print("This is a manual diagnostic, NOT a formal compatibility certification.");
    backend.write(output);
    backend.flush();
}

} // namespace

int main()
{
    try
    {
        AnsiTerminalBackend backend;
        const auto caps = backend.capabilities();
        std::string last_key = "(none)";
        std::string last_mouse = "(none)";
        std::size_t paste_bytes = 0;
        bool seen_mouse = false;
        bool seen_paste = false;
        bool seen_resize = false;
        bool dirty = true;

        for (;;)
        {
            if (dirty)
            {
                draw(backend,caps,last_key,last_mouse,paste_bytes,seen_mouse,seen_paste,seen_resize);
                dirty = false;
            }
            if (const auto event = backend.pollEvent())
            {
                if (event->key == Key::Escape || (event->key == Key::Character && event->character == U'q'))
                    break;
                switch (event->key)
                {
                    case Key::Mouse:
                        // Motion may produce many events; only redraw at the loop cadence.
                        last_mouse = mouseName(*event);
                        seen_mouse = true;
                        break;
                    case Key::Paste:
                        paste_bytes = event->text.size();
                        seen_paste = true;
                        last_key = "Paste event";
                        break;
                    case Key::Resize:
                        seen_resize = true;
                        last_key = "Resize event";
                        break;
                    default:
                        last_key = keyName(*event);
                        break;
                }
                dirty = true;
            }
            else
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(15));
            }
        }
    }
    catch (const std::exception& error)
    {
        std::cerr << "Terminal compatibility check failed: " << error.what() << '\n';
        return 1;
    }
}
