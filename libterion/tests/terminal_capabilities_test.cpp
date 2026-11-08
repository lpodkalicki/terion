/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <terion/terion.h>
#include <gtest/gtest.h>

using namespace terion;

TEST(TerminalCapabilities, Behavior)
{
    EXPECT_TRUE(!AnsiTerminalBackend::detectCapabilities("Apple_Terminal").mousePointerShape);
    EXPECT_TRUE(!AnsiTerminalBackend::detectCapabilities("vscode").mousePointerShape);
    EXPECT_TRUE(!AnsiTerminalBackend::detectCapabilities("iTerm.app").mousePointerShape);
    EXPECT_TRUE(AnsiTerminalBackend::detectCapabilities("ghostty").mousePointerShape);
    EXPECT_TRUE(AnsiTerminalBackend::detectCapabilities("kitty").mousePointerShape);
    EXPECT_TRUE(AnsiTerminalBackend::detectCapabilities("Apple_Terminal", "1").mousePointerShape);
    EXPECT_TRUE(!AnsiTerminalBackend::detectCapabilities("kitty", "0").mousePointerShape);
    auto pointer_backend = std::make_unique<HeadlessTerminalBackend>(Size{.width = 30,.height = 7});
    auto* pointer_output = pointer_backend.get();
    Application pointer_app(std::move(pointer_backend));
    pointer_app.render([] {
        return Column({.children = {
            Typography({.text = "Label"}),
            TextField({.value = "Edit",.key = "edit",.onChange = [](std::string) {},
                .size = ButtonSize::Compact}),
            Button({.text = "Click",.key = "click",.onClick = [] {},.size = ButtonSize::Compact}),
            Box({.children = {Typography({.text = "Box"})},.layout = {.width = 20,.height = 3}})
        }});
    });
    pointer_app.renderOnce();
    pointer_output->clearOutput();
    pointer_app.dispatchEvent(Event{.key = Key::Mouse,.mouse = {
        .position = {.x = 1,.y = 0},.action = MouseAction::Move}});
    EXPECT_NE(pointer_output->output().find("\033]22;text\033\\"),std::string::npos);
    pointer_output->clearOutput();
    pointer_app.dispatchEvent(Event{.key = Key::Mouse,.mouse = {
        .position = {.x = 15,.y = 0},.action = MouseAction::Move}});
    EXPECT_NE(pointer_output->output().find("\033]22;default\033\\"),std::string::npos);
    pointer_output->clearOutput();
    pointer_app.dispatchEvent(Event{.key = Key::Mouse,.mouse = {
        .position = {.x = 2,.y = 1},.action = MouseAction::Move}});
    EXPECT_NE(pointer_output->output().find("\033]22;text\033\\"),std::string::npos);
    pointer_output->clearOutput();
    pointer_app.dispatchEvent(Event{.key = Key::Mouse,.mouse = {
        .position = {.x = 2,.y = 2},.action = MouseAction::Move}});
    EXPECT_NE(pointer_output->output().find("\033]22;pointer\033\\"),std::string::npos);
    pointer_output->clearOutput();
    pointer_app.dispatchEvent(Event{.key = Key::Mouse,.mouse = {
        .position = {.x = 2,.y = 2},.action = MouseAction::Move}});
    EXPECT_TRUE(pointer_output->output().empty());
    pointer_app.dispatchEvent(Event{.key = Key::Mouse,.mouse = {
        .position = {.x = 15,.y = 4},.action = MouseAction::Move}});
    EXPECT_NE(pointer_output->output().find("\033]22;default\033\\"),std::string::npos);

}
