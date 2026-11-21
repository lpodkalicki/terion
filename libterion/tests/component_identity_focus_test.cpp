/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <terion/terion.h>
#include <gtest/gtest.h>

using namespace terion;

TEST(ComponentIdentity, KeysNeverBecomePublicFocusIds)
{
    Application app(std::make_unique<HeadlessTerminalBackend>(Size{48,12}));
    app.render([] { return Button({.text = "Save",.key = "reconciliation-save",
        .id = "save",.on_click = [] {}}); });
    app.renderOnce();
    EXPECT_FALSE(app.focus("reconciliation-save"));
    ASSERT_TRUE(app.focus("save"));
    EXPECT_TRUE(app.hasFocus("save"));
    EXPECT_EQ(app.focusedId(),"save");
    EXPECT_FALSE(app.hasFocus("reconciliation-save"));
}

TEST(ComponentIdentity, PublicIdDoesNotAlterKeyedReconciliation)
{
    Application app(std::make_unique<HeadlessTerminalBackend>(Size{48,12}));
    State<bool> change_id{false};
    app.render([&] { return TextField({.default_value = "untouched",
        .key = "stable-reconcile-key",.id = change_id.get() ? "new-id" : "old-id"}); });
    app.renderOnce();
    const auto runtime = app.reconciler().root()->id();
    ASSERT_TRUE(app.focus("old-id"));
    change_id.set(true);
    app.renderOnce();
    EXPECT_EQ(app.reconciler().root()->id(),runtime);
    EXPECT_FALSE(app.focus("old-id"));
    EXPECT_TRUE(app.focus("new-id"));
    EXPECT_EQ(app.focusedId(),"new-id");
}

TEST(ComponentIdentity, StablePublicIdRestoresFocusAfterKeyChange)
{
    Application app(std::make_unique<HeadlessTerminalBackend>(Size{48,12}));
    State<bool> remount{false};
    app.render([&] { return Column({.children = {
        Button({.text = "One",.key = remount.get() ? "new" : "old",
            .id = "public-one",.on_click = [] {}}),
        Button({.text = "Two",.key = "two",.id = "public-two",.on_click = [] {}})
    }}); });
    app.renderOnce();
    ASSERT_TRUE(app.focus("public-one"));
    const auto before = app.reconciler().root()->children()[0]->id();
    remount.set(true);
    app.renderOnce();
    EXPECT_NE(app.reconciler().root()->children()[0]->id(),before);
    EXPECT_TRUE(app.hasFocus("public-one"));
}

TEST(ComponentIdentity, KeyedReorderPreservesFocusedPublicId)
{
    Application app(std::make_unique<HeadlessTerminalBackend>(Size{48,12}));
    State<bool> reversed{false};
    app.render([&] {
        auto first = Button({.text = "First",.key = "key-first",.id = "first",
            .on_click = [] {}});
        auto second = Button({.text = "Second",.key = "key-second",.id = "second",
            .on_click = [] {}});
        std::vector<Component> children;
        if (reversed.get()) { children.push_back(second); children.push_back(first); }
        else { children.push_back(first); children.push_back(second); }
        return Column({.children = std::move(children)});
    });
    app.renderOnce();
    ASSERT_TRUE(app.focus("second"));
    const auto before = app.reconciler().root()->children()[1]->id();
    reversed.set(true);
    app.renderOnce();
    EXPECT_EQ(app.reconciler().root()->children()[0]->id(),before);
    EXPECT_TRUE(app.hasFocus("second"));
    EXPECT_EQ(app.focusedId(),"second");
}

TEST(ComponentIdentity, ScopeAndFocusedShortcutsUsePublicIds)
{
    Application app(std::make_unique<HeadlessTerminalBackend>(Size{48,12}));
    app.render([] { return FocusScope({.children = {
        Button({.text = "Run",.key = "private-action",.id = "run",.on_click = [] {}})
    },.key = "private-scope",.id = "workspace",.active = true}); });
    app.renderOnce();
    int scope_hits = 0;
    int control_hits = 0;
    int key_hits = 0;
    app.addShortcut({.key = Key::F1},[&] { ++scope_hits; },
        {.scope = ShortcutScope::FocusScope,.target = "workspace"});
    app.addShortcut({.key = Key::F2},[&] { ++control_hits; },
        {.scope = ShortcutScope::FocusedControl,.target = "run"});
    app.addShortcut({.key = Key::F3},[&] { ++key_hits; },
        {.scope = ShortcutScope::FocusedControl,.target = "private-action"});
    EXPECT_TRUE(app.focus("run"));
    EXPECT_FALSE(app.focus("private-action"));
    EXPECT_TRUE(app.dispatchEvent(Event{.key = Key::F1}));
    EXPECT_TRUE(app.dispatchEvent(Event{.key = Key::F2}));
    EXPECT_FALSE(app.dispatchEvent(Event{.key = Key::F3}));
    EXPECT_EQ(scope_hits,1);
    EXPECT_EQ(control_hits,1);
    EXPECT_EQ(key_hits,0);
}

TEST(ComponentIdentity, FormFieldsUseFieldNamesAsPublicIds)
{
    Application app(std::make_unique<HeadlessTerminalBackend>(Size{48,12}));
    app.render([] { return Form({.children = {
        FormField({.name = "hostname",.control = TextField({.key = "internal-input"})})
    }}); });
    app.renderOnce();
    EXPECT_TRUE(app.focus("hostname"));
    EXPECT_TRUE(app.hasFocus("hostname"));
    EXPECT_FALSE(app.focus("internal-input"));
    EXPECT_TRUE(app.focus("form-submit"));
}

TEST(ComponentIdentity, RejectsDuplicatePublicIdsWithinOneScope)
{
    Application app(std::make_unique<HeadlessTerminalBackend>(Size{48,12}));
    app.render([] { return Column({.children = {
        Button({.text = "First",.key = "first",.id = "duplicate",.on_click = [] {}}),
        Button({.text = "Second",.key = "second",.id = "duplicate",.on_click = [] {}})
    }}); });
    EXPECT_THROW(app.renderOnce(),std::invalid_argument);
}

TEST(ComponentIdentity, FluentIdentificationWorksOnComponents)
{
    Application app(std::make_unique<HeadlessTerminalBackend>(Size{48,12}));
    app.render([] { return Button({.text = "Save",.key = "private",
        .on_click = [] {}}).identified("save"); });
    app.renderOnce();
    EXPECT_TRUE(app.focus("save"));
}
