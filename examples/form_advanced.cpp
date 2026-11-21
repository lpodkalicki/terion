/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <memory>
#include <optional>
#include <string>
#include <terion/terion.h>

using namespace terion;

int main()
{
    Application app(std::make_unique<AnsiTerminalBackend>());
    auto controller = std::make_shared<FormController>();
    State<FormStatus> status{FormStatus{}};
    State<std::string> message{"Fill in the form. Tab/Shift+Tab to navigate."};

    app.render([&] {
        const auto current = status.get();
        return Column({.children = {
            Typography({.text = "#136c | Form Validation 2.0",
                .variant = TypographyVariant::H2}),
            Typography({.text = "OnBlur: errors appear after leaving a field or submitting."}),
            Form({
                .children = {
                    FormField({.name = "host",.label = "Host *",.required = true,
                        .control = TextField({.default_value = "server.local",
                            .placeholder = "Enter host",.layout = {.width = 38}})}),
                    FormField({.name = "mirror",.label = "Confirm host *",.required = true,
                        .control = TextField({.placeholder = "Repeat host",
                            .layout = {.width = 38}}),
                        .validator = [](const FormValue& value,const FormValues& values)
                            -> std::optional<std::string> {
                            if (std::get<std::string>(value) != values.get<std::string>("host"))
                                return "Must match the host above";
                            return std::nullopt;
                        }}),
                    FormField({.name = "port",.label = "Port *",.required = true,
                        .control = NumberField({.value = 8080,.min_value = 1024,
                            .max_value = 65535,.layout = {.width = 15}})}),
                    FormField({.name = "enabled",
                        .control = Checkbox({.label = "Enable connection",.default_checked = true})})
                },
                .key = "connection-form",
                .layout = {.padding = 1,.gap = 1},
                .submit_text = "Save connection",
                .validation_mode = FormValidationMode::OnBlur,
                .controller = controller,
                .on_state_change = [&](FormStatus next) { status.set(next); },
                .on_invalid = [&](const std::string& name) {
                    (void)app.focus(name); // FormField supplies the public focus id.
                    message.set("Validation failed at: " + name);
                },
                .on_submit = [&](const FormValues& values) {
                    message.set("Saved " + values.get<std::string>("host") + ":" +
                        std::to_string(values.get<int>("port")));
                },
                .on_reset = [&] { message.set("Form reset"); }
            }),
            Typography({.text = std::string("dirty=") + (current.dirty ? "yes" : "no") +
                " | valid=" + (current.valid ? "yes" : "no") +
                " | touched=" + std::to_string(current.touched_count) +
                " | errors=" + std::to_string(current.error_count) +
                " | submits=" + std::to_string(current.submit_count)}),
            Row({.children = {
                Button({.text = "Validate now",.on_click = [&,controller] {
                    if (!controller->ready()) return;
                    const bool valid = controller->validate();
                    if (!valid)
                    {
                        (void)app.focus(controller->firstError());
                        message.set("Fix: " + controller->firstError());
                    }
                    else message.set("All fields are valid");
                }}),
                Button({.text = "Fill demo",.on_click = [&,controller] {
                    if (!controller->ready()) return;
                    controller->setValue("host",std::string("example.org"));
                    controller->setValue("mirror",std::string("example.org"));
                }})
            },.layout = {.gap = 1}}),
            Typography({.text = message.get(),.color = ColorVariant::Info}),
            Typography({.text = "Tab: next | Shift+Tab: previous | Esc: quit"})
        },.layout = {.padding = 1,.gap = 1}});
    });
    return app.run();
}
