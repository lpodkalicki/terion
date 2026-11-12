/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <memory>
#include <string>
#include <terion/terion.h>

using namespace terion;

int main()
{
    Application app(std::make_unique<AnsiTerminalBackend>());
    State<std::string> result{std::string("Fill in the fields and press Submit")};

    app.render([&] {
        return Column({.children = {
            Typography({.text = "Form & validation — patch #126",.variant = TypographyVariant::H2}),
            Form({.children = {
                    FormField({.name = "hostname",.label = "Hostname *",.required = true,
                        .control = TextField({.default_value = "server.local",.placeholder = "Hostname",
                            .key = "hostname-input",.layout = {.width = 35}})}),
                    FormField({.name = "port",.label = "Port *",.required = true,
                        .control = NumberField({.value = 8080,.key = "port-input",.min_value = 1,.max_value = 65535,.layout = {.width = 20},
                            .validator = [](double port) -> std::optional<std::string> {
                                if (port < 1024) return "Port must be at least 1024";
                                return std::nullopt;
                            }})}),
                    FormField({.name = "enabled",.control = Checkbox({.label = "Enabled",
                        .key = "enabled-input",.default_checked = true})})
                },
                .key = "settings",.layout = {.padding = 1,.gap = 1},
                .on_submit = [&](const FormValues& values) {
                    result.set("Saved: " + values.get<std::string>("hostname") + ":" +
                        std::to_string(values.get<int>("port")));
                },
                .on_reset = [&] { result.set("Form reset"); }
            }),
            Typography({.text = result.get()})
        },.layout = {.padding = 1,.gap = 1}});
    });
    return app.run();
}
