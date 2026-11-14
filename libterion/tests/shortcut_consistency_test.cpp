/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <terion/terion.h>
#include <gtest/gtest.h>

using namespace terion;

namespace {

Event shortcutEvent(const char32_t character)
{
    return {.key = Key::Character,.character = character,.ctrl = true};
}

} /* End of anonymous namespace */

TEST(ShortcutConsistency, InputDecoderSupportsFunctionKeysAndModifiers)
{
    InputDecoder decoder;
    decoder.feed("\033OP\033[1;2Q\033[15~\033[24~\033[1;2D\033[1;6C");
    EXPECT_EQ(decoder.nextEvent()->key,Key::F1);
    const auto f2 = decoder.nextEvent();
    ASSERT_TRUE(f2);
    EXPECT_EQ(f2->key,Key::F2);
    EXPECT_TRUE(f2->shift);
    EXPECT_EQ(decoder.nextEvent()->key,Key::F5);
    EXPECT_EQ(decoder.nextEvent()->key,Key::F12);
    const auto left = decoder.nextEvent();
    ASSERT_TRUE(left);
    EXPECT_EQ(left->key,Key::Left);
    EXPECT_TRUE(left->shift);
    const auto right = decoder.nextEvent();
    ASSERT_TRUE(right);
    EXPECT_EQ(right->key,Key::Right);
    EXPECT_TRUE(right->ctrl);
    EXPECT_TRUE(right->shift);
}

TEST(ShortcutConsistency, CsiUAndXtermModifiedKeys)
{
    InputDecoder decoder;
    decoder.feed("\033[65;5u\033[9;2u\033[27;5;99~\033[1;2Z");
    const auto ctrl_a = decoder.nextEvent();
    ASSERT_TRUE(ctrl_a);
    EXPECT_EQ(ctrl_a->key,Key::Character);
    EXPECT_EQ(ctrl_a->character,U'a');
    EXPECT_TRUE(ctrl_a->ctrl);
    const auto tab = decoder.nextEvent();
    ASSERT_TRUE(tab);
    EXPECT_EQ(tab->key,Key::BackTab);
    EXPECT_TRUE(tab->shift);
    const auto ctrl_c = decoder.nextEvent();
    ASSERT_TRUE(ctrl_c);
    EXPECT_EQ(ctrl_c->character,U'c');
    EXPECT_TRUE(ctrl_c->ctrl);
    const auto back_tab = decoder.nextEvent();
    ASSERT_TRUE(back_tab);
    EXPECT_EQ(back_tab->key,Key::BackTab);
    EXPECT_TRUE(back_tab->shift);
    EXPECT_FALSE(decoder.nextEvent());
}

TEST(ShortcutConsistency, AltUnicodeHandlesSplitUtf8Sequences)
{
    InputDecoder decoder;
    decoder.feed("\033\xC5");
    EXPECT_FALSE(decoder.nextEvent());
    decoder.feed("\x81");
    const auto letter = decoder.nextEvent();
    ASSERT_TRUE(letter);
    EXPECT_EQ(letter->key,Key::Character);
    EXPECT_EQ(letter->character,U'Ł');
    EXPECT_TRUE(letter->alt);
    EXPECT_FALSE(decoder.nextEvent());
}

TEST(ShortcutConsistency, ShiftTabAndModifiedCharactersMatchAcrossProtocols)
{
    const Shortcut shift_tab{.key = Key::Tab,.shift = true};
    EXPECT_TRUE(shift_tab.matches(Event{.key = Key::BackTab}));
    EXPECT_TRUE(shift_tab.matches(Event{.key = Key::Tab,.shift = true}));
    EXPECT_FALSE(shift_tab.matches(Event{.key = Key::Tab}));
    const Shortcut ctrl_c{.key = Key::Character,.character = U'c',.ctrl = true};
    EXPECT_TRUE(ctrl_c.matches(Event{.key = Key::Character,.character = U'C',.ctrl = true}));
    EXPECT_FALSE(ctrl_c.matches(Event{.key = Key::Character,.character = U'C'}));
}

TEST(ShortcutConsistency, FocusTraversalAcceptsShiftTabButNotControlTab)
{
    FocusManager focus;
    focus.add("one",[](const Event&) { return false; });
    focus.add("two",[](const Event&) { return false; });
    EXPECT_TRUE(focus.dispatch({.key = Key::Tab}));
    EXPECT_EQ(focus.current(),"two");
    EXPECT_TRUE(focus.dispatch({.key = Key::Tab,.shift = true}));
    EXPECT_EQ(focus.current(),"one");
    EXPECT_FALSE(focus.dispatch({.key = Key::Tab,.ctrl = true}));
    EXPECT_EQ(focus.current(),"one");
    EXPECT_FALSE(focus.dispatch({.key = Key::BackTab,.alt = true}));
    EXPECT_EQ(focus.current(),"one");
}

TEST(ShortcutConsistency, EditingKeysTakePrecedenceOverGlobalShortcuts)
{
    Clipboard::clear();
    Application app(std::make_unique<HeadlessTerminalBackend>(Size{40,8}));
    app.render([] { return TextArea({.value = "hello",.key = "editor",.layout = {.width = 30,.height = 6}}); });
    app.renderOnce();
    int global_copies = 0;
    app.addShortcut({.key = Key::Character,.character = U'c',.ctrl = true},
        [&] { ++global_copies; });
    EXPECT_TRUE(app.dispatchEvent(shortcutEvent(U'a')));
    EXPECT_TRUE(app.dispatchEvent(shortcutEvent(U'c')));
    EXPECT_EQ(Clipboard::get(),"hello");
    EXPECT_EQ(global_copies,0);
}

TEST(ShortcutConsistency, BeforeFocusedCanExplicitlyOverrideEditingKeys)
{
    Clipboard::clear();
    Application app(std::make_unique<HeadlessTerminalBackend>(Size{40,8}));
    app.render([] { return TextField({.value = "hello",.key = "editor"}); });
    app.renderOnce();
    int count = 0;
    app.addShortcut({.key = Key::Character,.character = U'c',.ctrl = true},
        [&] { ++count; },{.priority = ShortcutPriority::BeforeFocused});
    EXPECT_TRUE(app.dispatchEvent(shortcutEvent(U'a')));
    EXPECT_TRUE(app.dispatchEvent(shortcutEvent(U'c')));
    EXPECT_EQ(count,1);
    EXPECT_TRUE(Clipboard::get().empty());
}

TEST(ShortcutConsistency, ShortcutCanTargetFocusedControlByComponentKey)
{
    Application app(std::make_unique<HeadlessTerminalBackend>(Size{60,10}));
    app.render([] { return Column({.children = {
        TextField({.key = "first"}),TextField({.key = "second"})
    }}); });
    app.renderOnce();
    int first = 0;
    int second = 0;
    app.addShortcut({.key = Key::Character,.character = U'k',.ctrl = true},
        [&] { ++first; },{.scope = ShortcutScope::FocusedControl,.target = "first"});
    app.addShortcut({.key = Key::Character,.character = U'k',.ctrl = true},
        [&] { ++second; },{.scope = ShortcutScope::FocusedControl,.target = "second"});
    ASSERT_TRUE(app.focus("first"));
    EXPECT_TRUE(app.dispatchEvent(shortcutEvent(U'k')));
    EXPECT_EQ(first,1);
    EXPECT_EQ(second,0);
    ASSERT_TRUE(app.focus("second"));
    EXPECT_TRUE(app.dispatchEvent(shortcutEvent(U'k')));
    EXPECT_EQ(first,1);
    EXPECT_EQ(second,1);
}

TEST(ShortcutConsistency, ShortcutCanTargetActiveFocusScopeByKey)
{
    Application app(std::make_unique<HeadlessTerminalBackend>(Size{60,10}));
    app.render([] { return FocusScope({.children = {
        TextField({.key = "editor"})
    },.key = "workspace",.active = true}); });
    app.renderOnce();
    int count = 0;
    app.addShortcut({.key = Key::Character,.character = U'k',.ctrl = true},
        [&] { ++count; },{.scope = ShortcutScope::FocusScope,.target = "workspace"});
    EXPECT_TRUE(app.dispatchEvent(shortcutEvent(U'k')));
    EXPECT_EQ(count,1);
    app.focus().setActiveScope("missing");
    EXPECT_FALSE(app.dispatchEvent(shortcutEvent(U'k')));
    EXPECT_EQ(count,1);
}

TEST(ShortcutConsistency, ShortcutCanRemoveItselfDuringDispatch)
{
    Application app(std::make_unique<HeadlessTerminalBackend>(Size{40,8}));
    int calls = 0;
    std::uint64_t id = 0;
    id = app.addShortcut({.key = Key::Character,.character = U'k',.ctrl = true},
        [&] { ++calls; (void)app.removeShortcut(id); });
    EXPECT_TRUE(app.dispatchEvent(shortcutEvent(U'k')));
    EXPECT_FALSE(app.dispatchEvent(shortcutEvent(U'k')));
    EXPECT_EQ(calls,1);
}

TEST(ShortcutConsistency, ModalDialogShieldsUnderlyingGlobalShortcuts)
{
    Application app(std::make_unique<HeadlessTerminalBackend>(Size{60,20}));
    app.render([] { return Dialog({.open = true,.title = "Modal",.children = {
        TextField({.key = "modal-input"})
    }}); });
    app.renderOnce();
    int global = 0;
    app.addShortcut({.key = Key::Character,.character = U'k',.ctrl = true},
        [&] { ++global; });
    EXPECT_TRUE(app.dispatchEvent(shortcutEvent(U'k')));
    EXPECT_EQ(global,0);
}
