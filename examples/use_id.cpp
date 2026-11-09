/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <terion/terion.h>

using namespace terion;

const auto IdentifiedPanel = component([] {
    const auto id = useId();
    return Typography({.text = "Instance id: " + id});
});

int main()
{
    Application app(std::make_unique<AnsiTerminalBackend>());
    app.render([] {
        return Column({.children = {
            IdentifiedPanel().keyed("first"),
            IdentifiedPanel().keyed("second")
        },.layout = {.padding = 2,.gap = 1}});
    });
    return app.run();
}
