/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <terion/terion.h>
#include <gtest/gtest.h>

using namespace terion;

TEST(Application, RendersAndRetainsKeyedComponents)
{
    auto backend = std::make_unique<HeadlessTerminalBackend>(Size{20,5});
    auto* observed = backend.get();
    Application app(std::move(backend));
    app.render([](Painter& p, Size) { p.text(0,0,"Terion"); });
    app.renderOnce();
    EXPECT_NE(observed->output().find("Terion"),std::string::npos);
    observed->clearOutput();
    std::string label = "First";
    app.render([&] {
        return Component::column({Component::text(label,"label")});
    });
    app.renderOnce();
    EXPECT_NE(observed->output().find("F"),std::string::npos);
    const auto label_id = app.reconciler().root()->children()[0]->id();
    ScreenBuffer component_buffer({.width = 20,.height = 5});
    Painter component_painter(component_buffer);
    app.reconciler().root()->paint(component_painter);
    EXPECT_EQ(component_buffer.at(0,0).grapheme,"F");
    label = "Next";
    observed->clearOutput();
    app.renderOnce();
    EXPECT_EQ(app.reconciler().root()->children()[0]->id(),label_id);
    EXPECT_NE(observed->output().find("N"),std::string::npos);
}

TEST(Application, StateInvalidatesOnlyObservedDependencies)
{
    auto backend = std::make_unique<HeadlessTerminalBackend>(Size{20,5});
    auto* observed = backend.get();
    Application app(std::move(backend));
    State<int> counter{0};
    observed->clearOutput();
    app.render([&] { return Component::text(std::to_string(counter.get()),"counter"); });
    EXPECT_TRUE(app.needsRender());
    app.renderOnce();
    EXPECT_TRUE(!app.needsRender());
    const auto counter_id = app.reconciler().root()->id();
    counter.set(1);
    EXPECT_TRUE(app.needsRender());
    observed->clearOutput();
    app.renderOnce();
    EXPECT_NE(observed->output().find("1"),std::string::npos);
    EXPECT_EQ(app.reconciler().root()->id(),counter_id);
    EXPECT_TRUE(!app.needsRender());
    counter.set(1);
    EXPECT_TRUE(!app.needsRender());
    counter.update([](const int value) { return value + 1; });
    EXPECT_TRUE(counter.get() == 2 && app.needsRender());
    State<bool> show{true};
    State<int> optional_value{10};
    app.render([&] {
        if (show.get()) return Component::text(std::to_string(optional_value.get()));
        return Component::text("hidden");
    });
    app.renderOnce();
    EXPECT_TRUE(!app.needsRender());
    show.set(false);
    EXPECT_TRUE(app.needsRender());
    app.renderOnce();
    EXPECT_TRUE(!app.needsRender());
    optional_value.set(11);
    EXPECT_TRUE(!app.needsRender());
    show.set(true);
    EXPECT_TRUE(app.needsRender());
}

TEST(Application, DispatchesFocusAndMouseActionsToButtons)
{
    auto backend = std::make_unique<HeadlessTerminalBackend>(Size{20,5});
    Application app(std::move(backend));
    int clicks = 0;
    app.render([&] {
        return Column({.children = {
            Typography({.text = "Actions",.variant = TypographyVariant::H2}),
            Button({.text = "First",.key = "first",.onClick = [&] { ++clicks; },.size = ButtonSize::Compact}),
            Button({.text = "Disabled",.key = "disabled",.disabled = true,.onClick = [&] { clicks += 100; },
                .size = ButtonSize::Compact}),
            Button({.text = "Second",.key = "second",.onClick = [&] { clicks += 10; },.size = ButtonSize::Compact})
        }});
    });
    app.renderOnce();
    const auto first_button_id = std::to_string(app.reconciler().root()->children()[1]->id());
    const auto second_button_id = std::to_string(app.reconciler().root()->children()[3]->id());
    EXPECT_EQ(app.focus().current(),first_button_id);
    EXPECT_TRUE(app.dispatchEvent(Event{.key = Key::Enter}));
    EXPECT_EQ(clicks,1);
    EXPECT_TRUE(app.dispatchEvent(Event{.key = Key::Tab}));
    EXPECT_EQ(app.focus().current(),second_button_id);
    EXPECT_TRUE(app.dispatchEvent(Event{.key = Key::Character,.character = U' '}));
    EXPECT_EQ(clicks,11);
    app.renderOnce();
    EXPECT_EQ(app.focus().current(),second_button_id);
    EXPECT_TRUE(app.dispatchEvent(Event{.key = Key::Mouse,.mouse = {
        .position = {.x = 2,.y = 1},.button = MouseButton::Left,.action = MouseAction::Press}}));
    EXPECT_EQ(app.focus().current(),first_button_id);
    EXPECT_TRUE(app.dispatchEvent(Event{.key = Key::Mouse,.mouse = {
        .position = {.x = 2,.y = 1},.button = MouseButton::Left,.action = MouseAction::Release}}));
    EXPECT_EQ(clicks,12);
    app.dispatchEvent(Event{.key = Key::Mouse,.mouse = {
        .position = {.x = 2,.y = 2},.button = MouseButton::Left,.action = MouseAction::Press}});
    app.dispatchEvent(Event{.key = Key::Mouse,.mouse = {
        .position = {.x = 2,.y = 2},.button = MouseButton::Left,.action = MouseAction::Release}});
    EXPECT_EQ(clicks,12);
    app.dispatchEvent(Event{.key = Key::Mouse,.mouse = {
        .position = {.x = 2,.y = 1},.button = MouseButton::Left,.action = MouseAction::Press}});
    app.dispatchEvent(Event{.key = Key::Mouse,.mouse = {
        .position = {.x = 40,.y = 10},.button = MouseButton::Left,.action = MouseAction::Release}});
    EXPECT_EQ(clicks,12);
    ScreenBuffer hover_buffer({.width = 20,.height = 5});
    Painter hover_painter(hover_buffer);
    app.reconciler().root()->paint(hover_painter,{},first_button_id);
    EXPECT_EQ(hover_buffer.at(0,1).grapheme,"▏");
}

TEST(Application, EditsUtf8TextAndMovesFocus)
{
    auto backend = std::make_unique<HeadlessTerminalBackend>(Size{20,5});
    Application app(std::move(backend));
    int clicks = 0;
    State<std::string> input{std::string{}};
    app.render([&] {
        return Column({.children = {
            TextField({.value = input.get(),.placeholder = "Name",.key = "name",
                .onChange = [&](std::string value) { input.set(std::move(value)); },.size = ButtonSize::Compact}),
            Button({.text = "OK",.key = "ok",.onClick = [&] { ++clicks; },.size = ButtonSize::Compact})
        }});
    });
    app.renderOnce();
    const auto field_id = std::to_string(app.reconciler().root()->children()[0]->id());
    EXPECT_EQ(app.focus().current(),field_id);
    EXPECT_TRUE(app.dispatchEvent(Event{.key = Key::Character,.character = U'Ł'}));
    EXPECT_EQ(input.get(),"Ł");
    app.renderOnce();
    EXPECT_TRUE(app.dispatchEvent(Event{.key = Key::Character,.character = U'x'}));
    EXPECT_EQ(input.get(),"Łx");
    app.renderOnce();
    app.dispatchEvent(Event{.key = Key::Left});
    app.dispatchEvent(Event{.key = Key::Backspace});
    EXPECT_EQ(input.get(),"x");
    app.renderOnce();
    EXPECT_EQ(app.reconciler().root()->children()[0]->id(),std::stoull(field_id));
    app.dispatchEvent(Event{.key = Key::Mouse,.mouse = {
        .position = {.x = 1,.y = 0},.button = MouseButton::Left,.action = MouseAction::Press}});
    app.dispatchEvent(Event{.key = Key::Character,.character = U'Z'});
    EXPECT_EQ(input.get(),"Zx");
    app.renderOnce();
    input.set("e\xCC\x81");
    app.renderOnce();
    app.dispatchEvent(Event{.key = Key::Mouse,.mouse = {
        .position = {.x = 3,.y = 0},.button = MouseButton::Left,.action = MouseAction::Press}});
    app.dispatchEvent(Event{.key = Key::Backspace});
    EXPECT_TRUE(input.get().empty());
    app.renderOnce();
    EXPECT_TRUE(app.dispatchEvent(Event{.key = Key::Tab}));
    EXPECT_TRUE(app.dispatchEvent(Event{.key = Key::Enter}));
    EXPECT_EQ(clicks,1);
}

TEST(Application, ClickingOutsideTextFieldRemovesFocusAndKeepsItRemovedAfterRender)
{
    auto backend = std::make_unique<HeadlessTerminalBackend>(Size{40,10});
    Application app(std::move(backend));
    State<std::string> input{std::string{}};
    app.render([&] {
        return Column({.children = {
            TextField({.value = input.get(),.onChange = [&](std::string value) {
                input.set(std::move(value)); },.size = ButtonSize::Compact}),
            Typography({.text = "Static label"})
        }});
    });
    app.renderOnce();
    EXPECT_FALSE(app.focus().current().empty());
    app.dispatchEvent(Event{.key = Key::Mouse,.mouse = {
        .position = {.x = 30,.y = 7},.button = MouseButton::Left,.action = MouseAction::Press}});
    EXPECT_TRUE(app.focus().current().empty());
    app.renderOnce();
    EXPECT_TRUE(app.focus().current().empty());
    EXPECT_FALSE(app.dispatchEvent(Event{.key = Key::Character,.character = U'x'}));
    EXPECT_TRUE(input.get().empty());
    EXPECT_TRUE(app.dispatchEvent(Event{.key = Key::Tab}));
    EXPECT_FALSE(app.focus().current().empty());
}

TEST(Application, DispatchesCustomEventsAndStopsHeadlessLoop)
{
    auto backend = std::make_unique<HeadlessTerminalBackend>(Size{20,5});
    auto* observed = backend.get();
    Application app(std::move(backend));
    app.render([](Painter&, Size) {});
    app.renderOnce();
    bool received = false;
    app.onEvent([&](const Event& event) { received = event.key == Key::Enter; return received; });
    EXPECT_TRUE(app.dispatchEvent(Event{.key = Key::Enter}));
    EXPECT_TRUE(received);
    received = false;
    app.focus().add("editor",[&](const Event& event) { return event.key == Key::Escape; });
    EXPECT_TRUE(app.dispatchEvent(Event{.key = Key::Escape}));
    EXPECT_TRUE(!received);
    EXPECT_TRUE(app.focus().remove("editor"));
    EXPECT_TRUE(app.dispatchEvent(Event{.key = Key::Escape}));
    // The headless backend allows run() to finish deterministically without a TTY.
    observed->push(Event{.key = Key::Escape});
    EXPECT_EQ(app.run(),0);
}

TEST(Application, FocusesKeyedComponentsAndRecoversWhenControlIsDisabled)
{
    Application app(std::make_unique<HeadlessTerminalBackend>(Size{40,10}));
    State<bool> available{true};
    app.render([&] {
        return Column({.children = {
            Button({.text = "First",.key = "first",.onClick = [] {}}),
            Button({.text = "Second",.key = "second",.disabled = !available.get(),.onClick = [] {}}),
            Button({.text = "Third",.key = "third",.onClick = [] {}})
        }});
    });
    app.renderOnce();
    EXPECT_TRUE(app.focus("second"));
    EXPECT_TRUE(app.hasFocus("second"));
    EXPECT_FALSE(app.hasFocus("first"));
    EXPECT_FALSE(app.focus("missing"));
    EXPECT_TRUE(app.hasFocus("second"));

    available.set(false);
    app.renderOnce();
    EXPECT_FALSE(app.hasFocus("second"));
    EXPECT_FALSE(app.focus("second"));
    EXPECT_FALSE(app.focused().empty());

    app.blur();
    EXPECT_TRUE(app.focused().empty());
    EXPECT_TRUE(app.dispatchEvent(Event{.key = Key::BackTab}));
    EXPECT_TRUE(app.hasFocus("third"));
    EXPECT_TRUE(app.focus("first"));
    EXPECT_TRUE(app.hasFocus("first"));
}

TEST(Application, RegistersAndRemovesShortcutsWithoutHardcodedBindings)
{
    Application app(std::make_unique<HeadlessTerminalBackend>(Size{40,10}));
    int count = 0;
    const auto id = app.addShortcut(Shortcut{.key = Key::Character,.character = U'f',.ctrl = true},
        [&] { ++count; });

    EXPECT_FALSE(app.dispatchEvent(Event{.key = Key::Character,.character = U'f'}));
    EXPECT_EQ(count,0);
    EXPECT_TRUE(app.dispatchEvent(Event{.key = Key::Character,.character = U'f',.ctrl = true}));
    EXPECT_EQ(count,1);
    EXPECT_TRUE(app.removeShortcut(id));
    EXPECT_FALSE(app.removeShortcut(id));
    EXPECT_FALSE(app.dispatchEvent(Event{.key = Key::Character,.character = U'f',.ctrl = true}));
    EXPECT_EQ(count,1);
}
