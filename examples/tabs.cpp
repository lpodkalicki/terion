/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <memory>
#include <terion/terion.h>

using namespace terion;

int main()
{
    Application app(std::make_unique<AnsiTerminalBackend>());
    State<std::string> active{"overview"};
    State<std::string> name{"Terion"};
    State<bool> notifications{true};
    app.render([&] {
        const auto page = active.get();
        return Paper({.children = {
            Typography({.text = "Tabs / TabPanel — Left/Right, Home/End, Tab, mouse",
                .variant = TypographyVariant::H2}),
            Tabs({.options = {{"Overview","overview"},{"Settings","settings"},
                {"Disabled","disabled",true},{"About","about"}},
                .value = page,.key = "main-tabs",.on_change = [&](std::string value) { active.set(std::move(value)); }}),
            TabPanel({.children = {
                Typography({.text = "Overview",.variant = TypographyVariant::H2}),
                Typography({.text = "This is a retained, composable tab panel."}),
                Button({.text = "Go to Settings",.key = "go-settings",.variant = ButtonVariant::Outlined,
                    .on_click = [&] { active.set("settings"); }})
            },.key = "overview-panel",.active = page == "overview",.layout = {.padding = 1,.gap = 1}}),
            TabPanel({.children = {
                Typography({.text = "Settings",.variant = TypographyVariant::H2}),
                TextField({.value = name.get(),.placeholder = "Application name",.key = "name",
                    .on_change = [&](std::string value) { name.set(std::move(value)); }}),
                Checkbox({.label = "Notifications",.key = "notifications",.checked = notifications.get(),
                    .on_change = [&](bool value) { notifications.set(value); }})
            },.key = "settings-panel",.active = page == "settings",.layout = {.padding = 1,.gap = 1}}),
            TabPanel({.children = {
                Typography({.text = "About Terion",.variant = TypographyVariant::H2}),
                Typography({.text = "C++20 declarative terminal UI"}),
                Typography({.text = "State is preserved when switching tabs."})
            },.key = "about-panel",.active = page == "about",.layout = {.padding = 1,.gap = 1}}),
            Typography({.text = "Active tab: " + page})
        },.layout = {.padding = 1,.gap = 1}});
    });
    return app.run();
}
