/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <terion/terion.h>

using namespace terion;

const auto Actions = component([] {
    return Fragment({.children = {
        Button({.text = "Save"}),
        Button({.text = "Cancel"})
    }});
});

int main()
{
    Application app(std::make_unique<AnsiTerminalBackend>());
    app.render([] {
        return Column({.children = {
            Typography({.text = "Fragment inserts no additional layout container"}),
            Actions(),
            Typography({.text = "End"})
        },.layout = {.padding = 2,.gap = 1}});
    });
    return app.run();
}
