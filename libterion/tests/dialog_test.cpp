/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <terion/terion.h>
#include <gtest/gtest.h>

using namespace terion;

TEST(Dialog, RestoresPreviousFocusAfterModalScopeCloses)
{
    State<bool> opened(false);
    Application app(std::make_unique<HeadlessTerminalBackend>(Size{.width = 60,.height = 20}));
    app.render([&] {
        return Column({.children = {
            Button({.text = "First",.key = "first",.onClick = [] {}}),
            Button({.text = "Open",.key = "open",.onClick = [&] { opened.set(true); }}),
            Dialog({.open = opened.get(),.title = "Dialog",.onClose = [&] { opened.set(false); },
                .children = {Button({.text = "Inside",.key = "inside",.onClick = [] {}})}})
        }});
    });
    app.renderOnce();
    ASSERT_TRUE(app.focus("open"));
    ASSERT_TRUE(app.dispatchEvent(Event{.key = Key::Enter}));
    app.renderOnce();
    EXPECT_TRUE(app.hasFocus("inside"));
    ASSERT_TRUE(app.dispatchEvent(Event{.key = Key::Escape}));
    app.renderOnce();
    EXPECT_TRUE(app.hasFocus("open"));
}

TEST(Dialog, DimsBackgroundWithoutErasingText)
{
    ScreenBuffer dim_buffer({.width = 10,.height = 2});
    Painter dim_painter(dim_buffer);
    dim_painter.text(0,0,"VISIBLE");
    const Color before_dim = dim_buffer.at(0,0).style.foreground;
    dim_painter.dim({.width = 10,.height = 2},Theme::dark().palette.background);
    EXPECT_EQ(dim_buffer.at(0,0).grapheme,"V");
    EXPECT_NE(dim_buffer.at(0,0).style.foreground,before_dim);
}

TEST(Dialog, DoesNotAffectParentLayout)
{
    Reconciler dialog_layout;
    dialog_layout.update(Column({.children = {
        Typography({.text = "Label"}),
        Dialog({.open = true,.title = "Hidden from layout"})
    },.layout = {.gap = 3}}));
    EXPECT_EQ(dialog_layout.root()->measure(Constraints{.maximum = {.width = 40,.height = 20}}).height,1);
}

TEST(Dialog, CapturesInputAndInvokesActions)
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

TEST(Dialog, CustomChildrenUseSpaceWithoutBuiltInActions)
{
    Reconciler reconciler;
    reconciler.update(Dialog({.open = true,.title = "Settings",.children = {
        Column({.children = {
            Typography({.text = "Name"}),
            TextField({.value = "Terion"}),
            Button({.text = "Save"})
        },.layout = {.gap = 1}})
    }}));
    auto* dialog = reconciler.root();
    const Size screen{.width = 70,.height = 24};
    dialog->layoutDialogChildren(screen);
    const Rect rect = dialog->dialogRect(screen);
    ASSERT_EQ(dialog->children().size(),1u);
    const auto* content = dialog->children()[0].get();
    EXPECT_GT(content->bounds().height,0);
    EXPECT_LE(content->bounds().y+content->bounds().height,rect.y+rect.height-1);
    ScreenBuffer buffer(screen);
    Painter painter(buffer);
    dialog->paintDialog(painter,screen,Theme::dark());
    EXPECT_EQ(buffer.at(content->children()[0]->bounds().x,
        content->children()[0]->bounds().y).grapheme,"N");
    EXPECT_NE(buffer.at(rect.x+2,rect.y+rect.height-2).grapheme,"C");
}

TEST(Dialog, CustomActionsCanBeExplicitlyEnabled)
{
    Reconciler reconciler;
    reconciler.update(Dialog({.open = true,.title = "Confirm",.children = {
        Typography({.text = "Details"})
    },.showActions = true}));
    auto* dialog = reconciler.root();
    const Size screen{.width = 60,.height = 18};
    dialog->layoutDialogChildren(screen);
    ScreenBuffer buffer(screen);
    Painter painter(buffer);
    dialog->paintDialog(painter,screen,Theme::dark());
    const Rect rect = dialog->dialogRect(screen);
    EXPECT_EQ(buffer.at(rect.x+2,rect.y+rect.height-2).grapheme,"C");
}

TEST(Dialog, ScrollsLongContentAndClipsHitTesting)
{
    std::vector<Component> children;
    for (int i = 0; i < 25; ++i)
        children.push_back(Button({.text = "Item " + std::to_string(i)}));
    Reconciler reconciler;
    reconciler.update(Dialog({.open = true,.title = "Items",.children = std::move(children)}));
    auto* dialog = reconciler.root();
    const Size screen{.width = 40,.height = 12};
    dialog->layoutDialogChildren(screen);
    ASSERT_EQ(dialog->children().size(),25u);
    const auto* first = dialog->children().front().get();
    const auto* last = dialog->children().back().get();
    const int first_y = first->bounds().y;
    EXPECT_LT(dialog->dialogRect(screen).height,last->bounds().y);
    EXPECT_TRUE(dialog->handleDialogMouse({.position = {.x = 20,.y = 6},
        .action = MouseAction::Scroll,.scrollY = -2},screen));
    EXPECT_LT(first->bounds().y,first_y);
    EXPECT_TRUE(dialog->scrollDialog(1000,screen));
    EXPECT_LE(last->bounds().y,screen.height);
    const auto rect = dialog->dialogRect(screen);
    EXPECT_EQ(dialog->dialogChildAt({.x = rect.x+2,.y = rect.y+1}),nullptr);
    EXPECT_FALSE(dialog->scrollDialog(1000,screen));
    EXPECT_TRUE(dialog->scrollDialog(-1000,screen));
    EXPECT_EQ(first->bounds().y,first_y);
}

TEST(Dialog, RevealsFocusedChildAfterScrolling)
{
    std::vector<Component> children;
    for (int i = 0; i < 20; ++i)
        children.push_back(Button({.text = "Action " + std::to_string(i)}));
    Reconciler reconciler;
    reconciler.update(Dialog({.open = true,.children = std::move(children)}));
    auto* dialog = reconciler.root();
    const Size screen{.width = 40,.height = 12};
    dialog->layoutDialogChildren(screen);
    const auto* last = dialog->children().back().get();
    EXPECT_TRUE(dialog->revealDialogFocus(last->id(),screen));
    const auto rect = dialog->dialogRect(screen);
    EXPECT_LT(last->bounds().y,rect.y+rect.height-1);
}

TEST(Dialog, UsesThemedScrollbarAndSupportsDragging)
{
    std::vector<Component> children;
    for (int i = 0; i < 30; ++i)
        children.push_back(Typography({.text = "Paragraph " + std::to_string(i)}));
    Reconciler reconciler;
    reconciler.update(Dialog({.open = true,.title = "Document",
        .children = std::move(children)}));
    auto* dialog = reconciler.root();
    const Size screen{.width = 50,.height = 12};
    dialog->layoutDialogChildren(screen);
    const Rect track = dialog->scrollbarTrack();
    const Rect initial_thumb = dialog->scrollbarThumb();
    ASSERT_GT(track.height,0);
    ASSERT_GT(initial_thumb.height,0);

    Theme theme = Theme::dark();
    theme.scrollbar.track = ".";
    theme.scrollbar.thumb = "@";
    ScreenBuffer buffer(screen);
    Painter painter(buffer);
    dialog->paintDialog(painter,screen,theme);
    EXPECT_EQ(buffer.at(initial_thumb.x,initial_thumb.y).grapheme,"@");
    EXPECT_EQ(buffer.at(track.x,track.y+track.height-1).grapheme,".");

    const Point start{.x = initial_thumb.x,.y = initial_thumb.y};
    EXPECT_TRUE(dialog->handleDialogMouse({.position = start,.button = MouseButton::Left,
        .action = MouseAction::Press},screen));
    EXPECT_TRUE(dialog->handleDialogMouse({.position = {.x = start.x,.y = track.y+track.height-1},
        .action = MouseAction::Move},screen));
    EXPECT_TRUE(dialog->handleDialogMouse({.position = {.x = start.x,.y = track.y+track.height-1},
        .button = MouseButton::Left,.action = MouseAction::Release},screen));
    EXPECT_GT(dialog->scrollbarThumb().y,initial_thumb.y);
}
