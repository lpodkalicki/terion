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
    State<std::string> selected{"node-a"};

    app.render([&] {
        const std::string current_name = name.get();
        const std::string selected_key = selected.get();
        return Paper({.children = {
            Typography({.text = "#139b | Controlled State & Selection",
                .variant = TypographyVariant::H2}),
            Typography({.text = "Controlled: parent State owns this value"}),
            TextField({.value = current_name,.key = "controlled-name",
                .on_change = [name](std::string value) mutable { name.set(std::move(value)); },
                .size = ControlSize::Medium}),
            Typography({.text = "Parent value: "+current_name}),
            Typography({.text = "Uncontrolled: edit without a callback; state survives rerenders"}),
            TextArea({.default_value = "This draft belongs to the editor.\nEdit me!",
                .key = "local-editor",.min_rows = 4,.soft_wrap = true,
                .layout = {.height = 6}}),
            Checkbox({.label = "Local toggle (no callback)",.key = "local-toggle",
                .default_checked = true}),
            Typography({.text = "Key-based Table selection: "+selected_key}),
            Table({.columns = {{"Name",18},{"Status",16}},
                .rows = {{"node-a",{"Node A","Online"}},
                    {"node-b",{"Node B","Idle"}},
                    {"node-c",{"Node C","Offline"}}},
                .key = "nodes",.layout = {.height = 6},
                .selected_key = selected_key,
                .on_selection_change = [selected](std::string value) mutable {
                    selected.set(std::move(value));
                }}),
            Typography({.text = "Tab/Shift+Tab: focus | Arrows: table selection | Esc: quit"})
        },.layout = {.padding = 1,.gap = 1}});
    });

    return app.run();
}
