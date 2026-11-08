/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <terion/terion.h>
#include <gtest/gtest.h>

using namespace terion;

TEST(EventPropagation, CapturesTargetsAndBubblesInOrder)
{
    std::vector<std::string> phases;
    int clicks = 0;
    Application app(std::make_unique<HeadlessTerminalBackend>(Size{.width = 40,.height = 10}));
    app.render([&] {
        return Column({.children = {
            Button({.text = "Action",.key = "action",.onClick = [&] { ++clicks; },
                .onEvent = [&](ComponentEvent& event) {
                    EXPECT_EQ(event.phase,EventPhase::Target);
                    phases.push_back("target");
                }})
        },.onEventCapture = [&](ComponentEvent& event) {
            EXPECT_EQ(event.phase,EventPhase::Capture);
            phases.push_back("capture");
        },.onEvent = [&](ComponentEvent& event) {
            EXPECT_EQ(event.phase,EventPhase::Bubble);
            phases.push_back("bubble");
        }});
    });
    app.renderOnce();
    EXPECT_TRUE(app.dispatchEvent(Event{.key = Key::Enter}));
    EXPECT_EQ(phases,(std::vector<std::string>{"capture","target","bubble"}));
    EXPECT_EQ(clicks,1);
}

TEST(EventPropagation, PreventDefaultBlocksWidgetAction)
{
    int clicks = 0;
    int bubbles = 0;
    Application app(std::make_unique<HeadlessTerminalBackend>(Size{.width = 40,.height = 10}));
    app.render([&] {
        return Column({.children = {
            Button({.text = "Action",.onClick = [&] { ++clicks; }})
        },.onEventCapture = [](ComponentEvent& event) {
            event.preventDefault();
            event.stopPropagation();
        },.onEvent = [&](ComponentEvent&) { ++bubbles; }});
    });
    app.renderOnce();
    EXPECT_TRUE(app.dispatchEvent(Event{.key = Key::Enter}));
    EXPECT_EQ(clicks,0);
    EXPECT_EQ(bubbles,0);
}

TEST(Focus, ActiveScopeRestrictsNavigationAndRestoresOuterTargets)
{
    FocusManager focus;
    focus.add("outside",[](const Event&) { return true; });
    focus.add("inside-a",[](const Event&) { return true; },"modal");
    focus.add("inside-b",[](const Event&) { return true; },"modal");
    EXPECT_EQ(focus.current(),"outside");
    EXPECT_FALSE(focus.focus("inside-a"));
    focus.setActiveScope("modal");
    EXPECT_EQ(focus.current(),"inside-a");
    EXPECT_FALSE(focus.focus("outside"));
    EXPECT_TRUE(focus.next());
    EXPECT_EQ(focus.current(),"inside-b");
    EXPECT_TRUE(focus.next());
    EXPECT_EQ(focus.current(),"inside-a");
    focus.setActiveScope({});
    EXPECT_EQ(focus.current(),"outside");
}

TEST(FocusScope, TrapsTabWithinActiveForm)
{
    State<bool> active(true);
    Application app(std::make_unique<HeadlessTerminalBackend>(Size{.width = 50,.height = 20}));
    app.render([&] {
        return Column({.children = {
            Button({.text = "Outside",.key = "outside",.onClick = [] {}}),
            FocusScope({.children = {
                Button({.text = "First",.key = "first",.onClick = [] {}}),
                Button({.text = "Second",.key = "second",.onClick = [] {}})
            },.key = "form",.active = active.get()})
        }});
    });
    app.renderOnce();
    const auto* root = app.reconciler().root();
    const auto* form = root->children()[1].get();
    EXPECT_EQ(app.focus().current(),std::to_string(form->children()[0]->id()));
    EXPECT_FALSE(app.focus().focus(std::to_string(root->children()[0]->id())));
    EXPECT_TRUE(app.dispatchEvent(Event{.key = Key::Tab}));
    EXPECT_EQ(app.focus().current(),std::to_string(form->children()[1]->id()));
    EXPECT_TRUE(app.dispatchEvent(Event{.key = Key::Tab}));
    EXPECT_EQ(app.focus().current(),std::to_string(form->children()[0]->id()));
    active.set(false);
    app.renderOnce();
    EXPECT_TRUE(app.focus().activeScope().empty());
    EXPECT_EQ(app.focus().current(),std::to_string(app.reconciler().root()->children()[0]->id()));
}

TEST(Dialog, ComposesInteractiveChildrenWithinModalFocusScope)
{
    State<bool> open(true);
    State<std::string> name(std::string{});
    int inside_clicks = 0;
    int outside_clicks = 0;
    Application app(std::make_unique<HeadlessTerminalBackend>(Size{.width = 80,.height = 24}));
    app.render([&] {
        return Column({.children = {
            Button({.text = "Outside",.key = "outside",.onClick = [&] { ++outside_clicks; }}),
            Dialog({.open = open.get(),.title = "Edit profile",.message = "Enter a name",
                .key = "profile",.onClose = [&] { open.set(false); },
                .children = {
                    TextField({.value = name.get(),.key = "name",.onChange = [&](std::string next) {
                        name.set(std::move(next));
                    },.size = ButtonSize::Compact}),
                    Button({.text = "Save",.key = "save",.onClick = [&] { ++inside_clicks; },
                        .size = ButtonSize::Compact})
                }})
        }});
    });
    app.renderOnce();
    auto* dialog = app.reconciler().root()->children()[1].get();
    ASSERT_TRUE(dialog->dialogOpen());
    ASSERT_EQ(dialog->children().size(),2u);
    EXPECT_EQ(app.focus().activeScope(),std::to_string(dialog->id()));
    EXPECT_EQ(app.focus().current(),std::to_string(dialog->children()[0]->id()));
    EXPECT_FALSE(app.focus().focus(std::to_string(app.reconciler().root()->children()[0]->id())));
    EXPECT_TRUE(app.dispatchEvent(Event{.key = Key::Character,.character = U'A'}));
    EXPECT_EQ(name.get(),"A");
    app.renderOnce();
    dialog = app.reconciler().root()->children()[1].get();
    EXPECT_TRUE(app.dispatchEvent(Event{.key = Key::Tab}));
    EXPECT_EQ(app.focus().current(),std::to_string(dialog->children()[1]->id()));
    EXPECT_TRUE(app.dispatchEvent(Event{.key = Key::Enter}));
    EXPECT_EQ(inside_clicks,1);
    const Rect save_bounds = dialog->children()[1]->bounds();
    EXPECT_TRUE(app.dispatchEvent(Event{.key = Key::Mouse,.mouse = {
        .position = {.x = save_bounds.x+1,.y = save_bounds.y},
        .button = MouseButton::Left,.action = MouseAction::Press}}));
    EXPECT_TRUE(app.dispatchEvent(Event{.key = Key::Mouse,.mouse = {
        .position = {.x = save_bounds.x+1,.y = save_bounds.y},
        .button = MouseButton::Left,.action = MouseAction::Release}}));
    EXPECT_EQ(inside_clicks,2);
    EXPECT_EQ(outside_clicks,0);
    open.set(false);
    app.renderOnce();
    EXPECT_TRUE(app.focus().activeScope().empty());
    EXPECT_EQ(app.focus().current(),std::to_string(app.reconciler().root()->children()[0]->id()));
}

TEST(Dialog, PaintsArbitraryNestedComponentsInsideOverlay)
{
    Reconciler reconciler;
    reconciler.update(Dialog({.open = true,.title = "Compose",.children = {
        Column({.children = {Typography({.text = "Nested content"})}})
    }}));
    auto* dialog = reconciler.root();
    const Size screen{.width = 60,.height = 20};
    dialog->layoutDialogChildren(screen);
    const auto* text_node = dialog->children()[0]->children()[0].get();
    EXPECT_GT(text_node->bounds().width,0);
    ScreenBuffer buffer(screen);
    Painter painter(buffer);
    dialog->paintDialog(painter,screen,Theme::dark());
    EXPECT_EQ(buffer.at(text_node->bounds().x,text_node->bounds().y).grapheme,"N");
}
