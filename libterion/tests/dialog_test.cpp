/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <terion/terion.h>
#include <gtest/gtest.h>

using namespace terion;

TEST(Dialog, Behavior)
{
    State<bool> show_dialog(true);
    int confirmations = 0;
    int background_clicks = 0;
    Application dialog_app(std::make_unique<HeadlessTerminalBackend>(Size{.width = 50,.height = 16}));
    dialog_app.render([&] {
        return Column({.children = {
            Button({.text = "Background",.key = "background",.onClick = [&] { ++background_clicks; }}),
            Dialog({.open = show_dialog.get(),.title = "Confirm",.message = "Proceed?",.key = "modal",
                .onConfirm = [&] { ++confirmations; show_dialog.set(false); },
                .onClose = [&] { show_dialog.set(false); }})
        }});
    });
    dialog_app.renderOnce();
    const auto* dialog_node = dialog_app.reconciler().root()->children()[1].get();
    const Rect dialog_rect = dialog_node->dialogRect(dialog_app.backend().size());
    EXPECT_TRUE(dialog_rect.width == 48 && dialog_rect.height == 9);
    ScreenBuffer dim_buffer({.width = 10,.height = 2});
    Painter dim_painter(dim_buffer);
    dim_painter.text(0,0,"VISIBLE");
    const Color before_dim = dim_buffer.at(0,0).style.foreground;
    dim_painter.dim({.width = 10,.height = 2},Theme::dark().palette.background);
    EXPECT_EQ(dim_buffer.at(0,0).grapheme,"V");
    EXPECT_NE(dim_buffer.at(0,0).style.foreground,before_dim);
    Reconciler dialog_layout;
    dialog_layout.update(Column({.children = {
        Typography({.text = "Label"}),
        Dialog({.open = true,.title = "Hidden from layout"})
    },.layout = {.gap = 3}}));
    EXPECT_EQ(dialog_layout.root()->measure(Constraints{.maximum = {.width = 40,.height = 20}}).height,1);
    EXPECT_TRUE(dialog_app.dispatchEvent(Event{.key = Key::Mouse,.mouse = {
        .position = {.x = 1,.y = 1},.button = MouseButton::Left,.action = MouseAction::Press}}));
    EXPECT_TRUE(background_clicks == 0 && !show_dialog.get());
    dialog_app.renderOnce();
    show_dialog.set(true);
    dialog_app.renderOnce();
    EXPECT_TRUE(dialog_app.dispatchEvent(Event{.key = Key::Tab}));
    EXPECT_TRUE(dialog_app.dispatchEvent(Event{.key = Key::Enter}));
    EXPECT_TRUE(confirmations == 1 && !show_dialog.get());
    show_dialog.set(true);
    dialog_app.renderOnce();
    EXPECT_TRUE(dialog_app.dispatchEvent(Event{.key = Key::Escape}));
    EXPECT_TRUE(!show_dialog.get());

}
