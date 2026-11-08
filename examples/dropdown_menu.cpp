/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <memory>
#include <terion/terion.h>

using namespace terion;

int main()
{
    Application app(std::make_unique<AnsiTerminalBackend>());
    State<std::string> selected{"No action yet"};
    app.render([&] {
        return Paper({.children = {
            Typography({.text = "DropdownMenu — Enter/Space, arrows, mouse, wheel, Esc",
                .variant = TypographyVariant::H2}),
            Row({.children = {
                DropdownMenu({.label = "File",.options = {
                    {"New", "new"}, {"Open", "open"}, {"Save", "save"},
                    {"Export", "export"}, {"Unavailable", "disabled",true}, {"Exit", "exit"}},
                    .key = "file",
                    .onSelect = [&](std::string action) { selected.set("File: " + action); }}),
                DropdownMenu({.label = "Edit",.options = {
                    {"Copy", "copy"}, {"Paste", "paste"}, {"Find", "find"}},
                    .key = "edit",
                    .variant = ButtonVariant::Outlined,
                    .onSelect = [&](std::string action) { selected.set("Edit: " + action); }}),
                DropdownMenu({.label = "View",.options = {
                    {"Zoom in", "zoom-in"}, {"Zoom out", "zoom-out"}},
                    .key = "view",.variant = ButtonVariant::Contained,
                    .onSelect = [&](std::string action) { selected.set("View: " + action); }}),
                DropdownMenu({.label = "Disabled",.key = "disabled",.disabled = true})
            },.layout = {.gap = 2}}),
            Typography({.text = "Last action: " + selected.get()}),
            Typography({.text = "The menu overlays this text when expanded."}),
            Typography({.text = "Try opening both menus and clicking outside."})
        },.layout = {.padding = 1,.gap = 1}});
    });
    return app.run();
}
