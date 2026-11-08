/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <terion/terion.h>

#include <string>

using namespace terion;

// Exactly the same C++ function syntax as for a stateless component.
Component Counter()
{
    auto [count,setCount] = useState(0);
    return Button({.text = "Count: " + std::to_string(count),
        .onClick = [setCount] { setCount([](int previous) { return previous + 1; }); }});
}

int main()
{
    Application app(std::make_unique<AnsiTerminalBackend>());
    State<bool> show_second(true);

    app.render([&] {
        std::vector<Component> children;
        children.emplace_back(Typography({.text = "Independent counter instances"}));
        children.emplace_back(ComponentOf<Counter>("first"));
        if (show_second.get()) children.emplace_back(ComponentOf<Counter>("second"));
        children.emplace_back(Button({.text = "Toggle second",.key = "toggle",
            .onClick = [&] { show_second.set(!show_second.get()); }}));
        return Column({.children = std::move(children),.layout = {.padding = 2,.gap = 1}});
    });
    return app.run();
}
