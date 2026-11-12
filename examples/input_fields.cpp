/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <memory>
#include <terion/terion.h>

using namespace terion;

int main()
{
    Application app(std::make_unique<AnsiTerminalBackend>());
    State<std::string> number{std::string("12.5")};
    State<std::string> secret{std::string{}};
    State<bool> reveal{false};
    app.render([&] {
        return Column({
            .children = {
                Typography({.text = "Input fields — patch #124",.variant = TypographyVariant::H2}),
                Typography({.text = "Number: Up/Down adjusts value; free-form decimal editing"}),
                NumberField({.value = number.get(),.placeholder = "Amount",.key = "number",
                    .min_value = -100,.max_value = 100,.step = 0.5,
                    .layout = {.width = 30},
                    .on_change = [&](std::string value) { number.set(std::move(value)); }}),
                Typography({.text = "Password: masked by default"}),
                PasswordField({.value = secret.get(),.placeholder = "Secret",.key = "password",
                    .reveal = reveal.get(),.layout = {.width = 30},
                    .on_change = [&](std::string value) { secret.set(std::move(value)); }}),
                Checkbox({.label = "Reveal password",.checked = reveal.get(),
                    .on_change = [&](bool checked) { reveal.set(checked); }}),
                Typography({.text = "Tab changes focus"})
            },
            .layout = {.padding = 1,.gap = 1}
        });
    });
    return app.run();
}
