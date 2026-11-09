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

const Context<ThemeState> themeContext({"system",{}});

// A custom hook is an ordinary C++ function called during component rendering.
ThemeState useTheme()
{
    return useContext(themeContext);
}

const auto ThemeLabel = component([] {
    const auto theme = useTheme();
    return Typography({.text = "Current theme: " + theme.name});
});

const auto ThemeToggle = component([] {
    const auto theme = useTheme();
    return Button({.text = "Toggle theme",.onClick = theme.toggle});
});

const auto ThemeProvider = component([] {
    auto [dark,setDark] = useState(false);
    ThemeState state{
        dark ? "dark" : "light",
        [setDark] { setDark([](bool previous) { return !previous; }); }
    };
    return ContextProvider(themeContext,std::move(state),std::vector<Component>{
        ThemeLabel(),
        ThemeToggle()
    });
});

int main()
{
    Application app(std::make_unique<AnsiTerminalBackend>());
    app.render([] {
        return Column({.children = {ThemeProvider()},.layout = {.padding = 2,.gap = 1}});
    });
    return app.run();
}
