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
    State<int> counter{0};

    app.render([&] {
        return Component::column({
            Component::text("Reactive Terion preview"),
            Component::text("Counter: " + std::to_string(counter.get()),"counter"),
            Component::text("Press Enter to increment, ESC to exit")
        },"root",LayoutStyle{.padding = 1,.gap = 1});
    });

    app.on_event([&](const Event& event) {
        if (event.key != Key::Enter) return false;
        counter.update([](const int value) { return value + 1; });
        return true;
    });

    return app.run();
}
