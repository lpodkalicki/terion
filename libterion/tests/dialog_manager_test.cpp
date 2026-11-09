/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <terion/terion.h>
#include <gtest/gtest.h>

using namespace terion;

TEST(DialogManager, OpensAndClosesFromDescendantComponent)
{
    auto dialogs = std::make_shared<DialogManager>();
    const auto trigger = component([] {
        auto manager = useDialogManager();
        return Button({.text = "Open",.onClick = [manager] {
            manager->open({.title = "Managed",.message = "Hello"});
        }});
    });
    Application app(std::make_unique<HeadlessTerminalBackend>(Size{.width = 65,.height = 18}));
    app.render([&] { return DialogProvider(dialogs,Column({.children = {trigger()}})); });
    app.renderOnce();
    EXPECT_FALSE(dialogs->isOpen());
    auto* root = app.reconciler().root();
    ASSERT_EQ(root->children().size(),1u);
    root->children()[0]->children()[0]->onClick()();
    EXPECT_TRUE(dialogs->isOpen());
    app.renderOnce();
    EXPECT_TRUE(app.dispatchEvent(Event{.key = Key::Escape}));
    EXPECT_FALSE(dialogs->isOpen());
}

TEST(DialogManager, CloseInvokesCustomCallback)
{
    auto dialogs = std::make_shared<DialogManager>();
    int callback_count = 0;
    dialogs->open({.title = "Example",.onClose = [&] { ++callback_count; }});
    Reconciler reconciler;
    reconciler.update(dialogs->render());
    ASSERT_TRUE(reconciler.root()->dialogOpen());
    EXPECT_TRUE(reconciler.root()->handleDialogKey(Event{.key = Key::Escape}));
    EXPECT_EQ(callback_count,1);
    EXPECT_FALSE(dialogs->isOpen());
}

TEST(DialogManager, ContextRequiredForHook)
{
    EXPECT_THROW((void)useDialogManager(),std::logic_error);
    EXPECT_THROW((void)DialogProvider(nullptr,Typography({.text = ""})),std::invalid_argument);
}
