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
    State<bool> enabled{true};
    State<std::string> name{""};
    State<int> clicks{0};

    app.render([&] {
        return Paper({.children = {
            Typography({.text = "Tooltip — hover over the controls with the mouse",
                .variant = TypographyVariant::H2}),
            Typography({.text = "Move the pointer away to dismiss the hint."}),
            Row({.children = {
                Tooltip({.child = Button({.text = "Save",.key = "save-button",
                    .variant = ButtonVariant::Contained,
                    .on_click = [&] { clicks.set(clicks.get()+1); }}),
                    .text = "Save the current document",.key = "save-tooltip"}),
                Tooltip({.child = Button({.text = "Export",.key = "export-button",
                    .on_click = [&] { clicks.set(clicks.get()+1); }}),
                    .text = "Export a copy of the document",.key = "export-tooltip"}),
                Tooltip({.child = Button({.text = "Unavailable",.disabled = true}),
                    .text = "This action is not available",.key = "disabled-button-tooltip"})
            },.layout = {.gap = 2}}),
            Tooltip({.child = Checkbox({.label = "Enable automatic backup",.key = "backup",
                .checked = enabled.get(),.on_change = [&](bool value) { enabled.set(value); }}),
                .text = "Automatically save changes every few minutes",.key = "backup-tooltip"}),
            Tooltip({.child = TextField({.value = name.get(),.placeholder = "Document name",
                .key = "document-name",.on_change = [&](std::string value) { name.set(std::move(value)); }}),
                .text = "Enter the name of your document",.key = "name-tooltip"}),
            Typography({.text = "Actions: " + std::to_string(clicks.get()) +
                " | Backup: " + (enabled.get() ? "ON" : "OFF")}),
            Typography({.text = "Try hovering near the right and bottom edges of the terminal."}),
            Typography({.text = "Tab / Shift+Tab changes focus; tooltips follow the mouse."})
        },.layout = {.padding = 1,.gap = 1}});
    });
    return app.run();
}
