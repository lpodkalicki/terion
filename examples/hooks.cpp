/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <terion/terion.h>

#include <string>

using namespace terion;

const auto Counter = component([] {
    auto [count,set_count] = useState(0);
    auto renders = useRef(0);
    ++*renders;
    const auto title = useMemo([count] { return "Count: " + std::to_string(count); },count);
    auto increment = useCallback([set_count] {
        set_count([](int previous) { return previous + 1; });
    });
    return Button({.text = title + " (renders: " + std::to_string(*renders) + ")",
        .on_click = std::move(increment)});
});

int main()
{
    Application app(std::make_unique<AnsiTerminalBackend>());
    State<bool> show_second(true);

    app.render([&] {
        std::vector<Component> children;
        children.emplace_back(Typography({.text = "Independent counter instances"}));
        children.emplace_back(Counter().keyed("first"));
        if (show_second.get()) children.emplace_back(Counter().keyed("second"));
        children.emplace_back(Button({.text = "Toggle second",.key = "toggle",
            .on_click = [&] { show_second.set(!show_second.get()); }}));
        return Column({.children = std::move(children),.layout = {.padding = 2,.gap = 1}});
    });
    return app.run();
}
