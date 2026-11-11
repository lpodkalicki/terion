/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <memory>
#include <terion/terion.h>

using namespace terion;

int main()
{
    Application app(std::make_unique<AnsiTerminalBackend>());
    State<int> clicks{0};
    app.render([&] {
        return Paper({
            .children = {
                Typography({.text = "Terion Buttons",.variant = TypographyVariant::H2}),
                Divider({.layout = {.width = 56}}),
                Typography({.text = "Variants  (Tab to focus, mouse to hover/click)"}),
                Row({.children = {
                    Button({.text = "Contained",.key = "contained",.variant = ButtonVariant::Contained,
                        .on_click = [&] { clicks.set(clicks.get() + 1); }}),
                    Button({.text = "Outlined",.key = "outlined",.variant = ButtonVariant::Outlined,
                        .on_click = [&] { clicks.set(clicks.get() + 1); }}),
                    Button({.text = "Text",.key = "text",.variant = ButtonVariant::Text,
                        .on_click = [&] { clicks.set(clicks.get() + 1); }})
                },.layout = {.gap = 2}}),
                Typography({.text = "Sizes"}),
                Row({.children = {
                    Button({.text = "Compact",.size = ButtonSize::Compact}),
                    Button({.text = "Medium",.variant = ButtonVariant::Contained}),
                    Button({.text = "Large",.variant = ButtonVariant::Outlined,.size = ButtonSize::Large})
                },.layout = {.gap = 2,.align_items = AlignItems::Center}}),
                Typography({.text = "Disabled"}),
                Row({.children = {
                    Button({.text = "Disabled",.variant = ButtonVariant::Contained,.disabled = true}),
                    Button({.text = "Disabled",.variant = ButtonVariant::Outlined,.disabled = true})
                },.layout = {.gap = 2}}),
                Typography({.text = "Clicked: " + std::to_string(clicks.get())})
            },
            .layout = {.padding = 1,.gap = 1}
        });
    });
    return app.run();
}
