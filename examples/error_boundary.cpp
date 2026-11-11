/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <terion/terion.h>
#include <stdexcept>

using namespace terion;

int main()
{
    Application app(std::make_unique<AnsiTerminalBackend>());
    State<bool> failing(false);

    const auto Panel = component([&] {
        if (failing.get()) throw std::runtime_error("Cannot load panel");
        return Typography({.text = "Panel ready"});
    });

    app.render([&] {
        return Column({.children = {
            Button({.text = "Toggle failure",.on_click = [&] { failing.set(!failing.get()); }}),
            ErrorBoundary({.child = Panel(),
                .fallback = [](const std::string& message) {
                    return Typography({.text = "Fallback: " + message});
                }})
        },.layout = {.padding = 2,.gap = 1}});
    });
    return app.run();
}
