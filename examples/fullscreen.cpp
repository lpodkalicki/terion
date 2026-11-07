/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <algorithm>
#include <memory>
#include <string>
#include <terion/terion.h>

using namespace terion;

int main()
{
    Application app(std::make_unique<AnsiTerminalBackend>());
    State<int> load{65};
    State<std::string> query{"Terion"};
    State<bool> light{false};

    app.render([&] {
        const Size screen = app.backend().size();
        const int sidebar_width = std::min(26,std::max(0,screen.width / 3));
        const int content_width = std::max(0,screen.width - sidebar_width - 2);
        return Column({
            .children = {
                Paper({
                    .children = {Row({
                        .children = {
                            Typography({.text = "TERION  /  Dashboard",.variant = TypographyVariant::H2}),
                            Typography({.text = "C++20 | MUI-inspired"})
                        },
                        .layout = {.width = screen.width,.justifyContent = JustifyContent::SpaceBetween}
                    })},
                    .layout = {.width = screen.width,.padding = 0}
                }),
                Divider({.layout = {.width = screen.width}}),
                Row({
                    .children = {
                        Card({
                            .title = "Navigation",
                            .children = {
                                Typography({.text = "Overview"}),
                                Divider({}),
                                Button({.text = "Dashboard",.variant = ButtonVariant::Contained}),
                                Button({.text = "Settings"}),
                                Button({.text = "Reports",.disabled = true}),
                                Typography({.text = "Search",.variant = TypographyVariant::Subtitle}),
                                TextField({.value = query.get(),.placeholder = "Type to search...",
                                    .key = "search",.onChange = [&](std::string text) { query.set(std::move(text)); }}),
                                Typography({.text = "Input: " + query.get()})
                            },
                            .layout = {.width = sidebar_width,.padding = 1,.gap = 1}
                        }),
                        Column({
                            .children = {
                                Grid({
                                    .children = {
                                        Paper({.children = {
                                            Typography({.text = "CPU  42%",.variant = TypographyVariant::Subtitle}),
                                            Progress({.value = 0.42,.layout = {.width = 16}})
                                        },.layout = {.padding = 1}}),
                                        Paper({.children = {
                                            Typography({.text = "RAM  68%",.variant = TypographyVariant::Subtitle}),
                                            Progress({.value = 0.68,.layout = {.width = 16}})
                                        },.layout = {.padding = 1}})
                                    },
                                    .layout = {.width = content_width,.gap = 1},.columns = 2
                                }),
                                Card({
                                    .title = "Transfer status",
                                    .children = {
                                        Typography({.text = "Simulated download progress"}),
                                        Progress({.value = load.get() / 100.0,
                                            .layout = {.width = std::max(0,content_width - 8)}}),
                                        Stack({
                                            .children = {
                                                Button({.text = "+10%",.key = "increase",
                                                    .variant = ButtonVariant::Contained,
                                                    .onClick = [&] { load.set(std::min(100,load.get() + 10)); }}),
                                                Button({.text = "Reset",.key = "reset",
                                                    .onClick = [&] { load.set(0); }})
                                            },
                                            .layout = {.gap = 2},.direction = StackDirection::Horizontal
                                        }),
                                        Box({.children = {Typography({.text = "Reactive State<T> + retained tree"})},
                                            .layout = {.padding = 1},.border = BorderStyle::Single})
                                    },
                                    .layout = {.width = content_width,.padding = 1,.gap = 1,.flex = 1}
                                })
                            },
                            .layout = {.gap = 1,.flex = 1}
                        })
                    },
                    .layout = {.gap = 1,.flex = 1,.alignItems = AlignItems::Stretch}
                }),
                Paper({
                    .children = {Row({
                        .children = {
                            Typography({.text = "Ready  |  Tab: focus  |  Mouse: click"}),
                            Button({.text = light.get() ? "Dark" : "Light",.key = "theme",
                                .variant = ButtonVariant::Text,.onClick = [&] {
                                    light.set(!light.get());
                                    app.setTheme(light.get() ? Theme::light() : Theme::dark());
                                }})
                        },.layout = {.width = screen.width,.justifyContent = JustifyContent::SpaceBetween}
                    })},
                    .layout = {.width = screen.width,.padding = 0}
                })
            },
            .layout = {.width = screen.width,.height = screen.height,.alignItems = AlignItems::Stretch}
        });
    });
    return app.run();
}
