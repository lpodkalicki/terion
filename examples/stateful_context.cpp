/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <terion/terion.h>

using namespace terion;

struct ThemeState
{
    std::string name;
    std::function<void()> toggle;
};

const Context<ThemeState> theme_context({"system",{}});

// A custom hook is an ordinary C++ function called during component rendering.
ThemeState useThemeState()
{
    return useContext(theme_context);
}

const auto ThemeLabel = component([] {
    const auto theme = useThemeState();
    return Typography({.text = "Current theme: " + theme.name});
});

const auto ThemeToggle = component([] {
    const auto theme = useThemeState();
    return Button({.text = "Toggle theme",.on_click = theme.toggle});
});

const auto StatefulThemeProvider = component([] {
    auto [dark,set_dark] = useState(false);
    ThemeState state{
        dark ? "dark" : "light",
        [set_dark] { set_dark([](bool previous) { return !previous; }); }
    };
    return ContextProvider(theme_context,std::move(state),std::vector<Component>{
        ThemeLabel(),
        ThemeToggle()
    });
});

int main()
{
    Application app(std::make_unique<AnsiTerminalBackend>());
    app.render([] {
        return Column({.children = {StatefulThemeProvider()},.layout = {.padding = 2,.gap = 1}});
    });
    return app.run();
}
