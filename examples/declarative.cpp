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
            Component::text("Terion declarative preview","title"),
            Component::text("Component + RenderNode + Reconciler"),
            Component::text("Press ESC to exit")
        });
    });
    return app.run();
}
