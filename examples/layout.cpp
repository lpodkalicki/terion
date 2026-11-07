/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <memory>
#include <terion/terion.h>

using namespace terion;

int main()
{
    Application app(std::make_unique<AnsiTerminalBackend>());
    app.render([] {
        return Component::column({
            Component::text("Terion layout preview","title"),
            Component::row({
                Component::box({Component::text("LEFT")},LayoutStyle{.width = 16,.padding = 2}),
                Component::box({Component::text("RIGHT")},LayoutStyle{.width = 16,.padding = 2})
            },"panels",LayoutStyle{.gap = 2}),
            Component::text("Press ESC to exit")
        },"root",LayoutStyle{.padding = 1,.gap = 1});
    });
    return app.run();
}
