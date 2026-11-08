/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <terion/terion.h>

using namespace terion;

int main()
{
    Application app(std::make_unique<AnsiTerminalBackend>());
    State<bool> dialog_open(false);
    State<int> confirmed(0);
    app.render([&] {
        return Column({.children = {
            Typography({.text = "Terion Dialog — Enter, Tab, Escape, click outside"}),
            Button({.text = "Open confirmation dialog",.key = "open",.variant = ButtonVariant::Contained,
                .onClick = [&] { dialog_open.set(true); }}),
            Typography({.text = "Confirmed: " + std::to_string(confirmed.get())}),
            Dialog({.open = dialog_open.get(),.title = "Confirm action",
                .message = "Do you want to continue?",.key = "confirm",.confirmText = "Continue",
                .cancelText = "Cancel",.onConfirm = [&] {
                    confirmed.set(confirmed.get()+1);
                    dialog_open.set(false);
                },.onClose = [&] { dialog_open.set(false); }})
        },.layout = {.padding = 2,.gap = 1}});
    });
    return app.run();
}
