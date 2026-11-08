/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <terion/terion.h>

#include <string>

using namespace terion;

namespace {

const Context<std::string> theme_context("system");

Component ThemeLabel()
{
    const auto theme = useContext(theme_context);
    return Typography({.text = "Theme: " + theme});
}

Component ThemePanel()
{
    // ThemePanel does not receive or forward any theme props.
    return Card({.title = "User panel",.children = {ComponentOf<ThemeLabel>("theme")}});
}

} /* End of anonymous namespace */

int main()
{
    Application app(std::make_unique<AnsiTerminalBackend>());
    State<bool> dark(false);

    app.render([&] {
        const std::string current = dark.get() ? "dark" : "light";
        return Column({.children = {
            Button({.text = "Toggle theme",.key = "toggle",
                .onClick = [&] { dark.set(!dark.get()); }}),
            ContextProvider(theme_context,current,ComponentOf<ThemePanel>("panel"),"provider"),
            ComponentOf<ThemeLabel>("fallback")
        },.layout = {.padding = 2,.gap = 1}});
    });
    return app.run();
}
