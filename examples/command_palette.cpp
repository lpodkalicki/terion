/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <terion/terion.h>

using namespace terion;

int main()
{
    Application app(std::make_unique<AnsiTerminalBackend>());
    State<bool> palette_open(false);
    State<std::string> query("");
    State<std::string> status("Press Ctrl+P to open the command palette");
    const std::vector<CommandPaletteAction> commands = {
        {.id = "open",.title = "Open file",.description = "Browse files",.shortcut = "Ctrl+O"},
        {.id = "reload",.title = "Reload configuration",.description = "Refresh settings",.shortcut = "F5"},
        {.id = "restart",.title = "Restart service",.description = "Restart the selected service"},
        {.id = "delete",.title = "Delete all",.description = "Unavailable action",.disabled = true}
    };

    app.addShortcut(Shortcut{.key = Key::Character,.character = U'p',.ctrl = true},[&] {
        query.set("");
        palette_open.set(true);
    });
    app.render([&] {
        return Column({.children = {
            Typography({.text = "CommandPalette example"}),
            Button({.text = "Open palette (Ctrl+P)",.on_click = [&] {
                query.set("");
                palette_open.set(true);
            }}),
            Typography({.text = status.get()}),
            CommandPalette({
                .open = palette_open.get(),
                .commands = commands,
                .query = query.get(),
                .on_query_change = [&](std::string value) { query.set(std::move(value)); },
                .on_execute = [&](std::string id) { status.set("Executed command: " + id); },
                .on_close = [&] { palette_open.set(false); }
            })
        },.layout = {.padding = 2,.gap = 1}});
    });
    return app.run();
}
