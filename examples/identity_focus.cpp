/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <memory>
#include <string>
#include <terion/terion.h>

using namespace terion;

int main()
{
    Application app(std::make_unique<AnsiTerminalBackend>());
    State<std::string> name{"Terion"};
    State<std::string> status{"F2: Name  F3: Save  Tab/Shift+Tab: navigate  Esc: quit"};

    app.addShortcut({.key = Key::F2},[&] {
        status.set(app.focus("name") ? "Focused: name" : "Name is unavailable");
    },{.priority = ShortcutPriority::BeforeFocused});
    app.addShortcut({.key = Key::F3},[&] {
        status.set(app.focus("save") ? "Focused: save" : "Save is unavailable");
    },{.priority = ShortcutPriority::BeforeFocused});

    app.render([&] {
        return Column({.children = {
            Typography({.text = "#139e  Component Identity & Focus",
                .variant = TypographyVariant::H2}),
            Typography({.text = "key retains state; id is used by focus and shortcuts."}),
            TextField({.value = name.get(),.key = "name-state",.id = "name",
                .on_change = [&](std::string value) { name.set(std::move(value)); }}),
            Button({.text = "Save",.key = "save-state",.id = "save",
                .on_click = [&] { status.set("Saved: " + name.get()); }}),
            Typography({.text = status.get()})
        },.layout = {.padding = 1,.gap = 1}});
    });
    return app.run();
}
