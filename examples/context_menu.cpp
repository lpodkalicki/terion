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
    State<std::string> selected{"report.pdf"};
    State<std::string> last_action{"No action yet"};

    app.render([&] {
        return Paper({.children = {
            Typography({.text = "ContextMenu — right-click to open",
                .variant = TypographyVariant::H2}),
            Typography({.text = "Right-click a button or the table below."}),
            Row({.children = {
                ContextMenu({.child = Button({.text = "File actions",.key = "actions-button",
                    .variant = ButtonVariant::Contained,
                    .onClick = [&] { last_action.set("Button clicked"); }}),
                    .options = {{"New file","new"},{"Import","import"},
                        {"Disabled action","disabled",true}},
                    .key = "button-context-menu",
                    .onSelect = [&](std::string action) { last_action.set("Button menu: " + action); }}),
                ContextMenu({.child = Button({.text = "More",.key = "more-button",
                    .onClick = [&] { last_action.set("More clicked"); }}),
                    .options = {{"Refresh","refresh"},{"Help","help"}},
                    .key = "more-context-menu",
                    .onSelect = [&](std::string action) { last_action.set("More menu: " + action); }})
            },.layout = {.gap = 2}}),
            Typography({.text = "Files (left-click to select, right-click for actions)",
                .variant = TypographyVariant::Subtitle}),
            ContextMenu({.child = Table({
                .columns = {{"Name",22},{"Type",10},{"Size",10,TableAlignment::Right}},
                .rows = {{"report.pdf",{"report.pdf","PDF","128 KB"}},
                    {"notes.txt",{"notes.txt","Text","3 KB"}},
                    {"photo.png",{"photo.png","Image","2 MB"}}},
                .value = selected.get(),.key = "files-table",
                .layout = {.height = 5},
                .onChange = [&](std::string value) { selected.set(std::move(value)); }}),
                .options = {{"Open","open"},{"Rename","rename"},
                    {"Delete...","delete"},{"Unavailable","disabled",true}},
                .key = "table-context-menu",
                .onSelect = [&](std::string action) {
                    last_action.set(action + ": " + selected.get());
                }}),
            Typography({.text = "Selected file: " + selected.get()}),
            Typography({.text = "Last action: " + last_action.get()}),
            Typography({.text = "Arrow keys / Enter to choose, Esc to close."}),
            Typography({.text = "Click outside the menu to dismiss it."})
        },.layout = {.padding = 1,.gap = 1}});
    });
    return app.run();
}
