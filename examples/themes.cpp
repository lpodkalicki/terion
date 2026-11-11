/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <terion/terion.h>
#include <array>

using namespace terion;

namespace {

struct ThemePreset
{
    const char* name;
    Theme (*make)();
};

constexpr std::array presets{
    ThemePreset{"Dark",Theme::dark},ThemePreset{"Light",Theme::light},
    ThemePreset{"Ocean",Theme::ocean},ThemePreset{"Forest",Theme::forest},
    ThemePreset{"Sunset",Theme::sunset},ThemePreset{"Violet",Theme::violet},
    ThemePreset{"Nord",Theme::nord},ThemePreset{"Dracula",Theme::dracula},
    ThemePreset{"Solarized",Theme::solarized}
};

const auto Preview = component([] {
    const auto theme = useTheme();
    return Column({.children = {
        Typography({.text = "Typography and components inherit the current theme"}),
        Button({.text = "Primary button",.variant = ButtonVariant::Contained}),
        Progress({.value = 0.65}),
        Typography({.text = "Background RGB: " + std::to_string(theme.palette.background.red) +
            ", " + std::to_string(theme.palette.background.green) +
            ", " + std::to_string(theme.palette.background.blue)})
    },.layout = {.gap = 1}});
});

} /* End of anonymous namespace */

int main()
{
    Application app(std::make_unique<AnsiTerminalBackend>());
    State<int> selected(0);
    app.render([&] {
        const auto index = static_cast<std::size_t>(selected.get());
        return ThemeProvider(presets[index].make(),Column({.children = {
            Typography({.text = std::string("Theme: ")+presets[index].name}),
            Row({.children = {
                Button({.text = "Previous",.on_click = [&] {
                    selected.update([](int n) { return (n+8)%9; });
                }}),
                Button({.text = "Next",.on_click = [&] {
                    selected.update([](int n) { return (n+1)%9; });
                }})
            },.layout = {.gap = 1}}),
            Preview()
        },.layout = {.padding = 2,.gap = 2}}));
    });
    return app.run();
}
