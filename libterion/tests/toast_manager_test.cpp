/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <chrono>
#include <gtest/gtest.h>
#include <terion/terion.h>

using namespace terion;

TEST(ToastManager, EnqueuesThreeAndPromotesPendingAfterExpiry)
{
    ToastManager manager;
    for (int i = 0; i < 5; ++i)
        (void)manager.enqueue({.message = std::to_string(i),.duration_ms = 100});
    EXPECT_EQ(manager.visibleCount(),3);
    EXPECT_EQ(manager.pendingCount(),2);
    EXPECT_TRUE(manager.tick(std::chrono::steady_clock::now()+std::chrono::seconds(1)));
    EXPECT_EQ(manager.visibleCount(),2);
    EXPECT_EQ(manager.pendingCount(),0);
}

TEST(ToastManager, PersistentNotificationWaitsUntilDismissed)
{
    ToastManager manager;
    const auto id = manager.enqueue({.message = "Persistent",.duration_ms = 0});
    EXPECT_FALSE(manager.tick(std::chrono::steady_clock::now()+std::chrono::hours(1)));
    EXPECT_EQ(manager.visibleCount(),1);
    EXPECT_TRUE(manager.dismiss(id));
    EXPECT_FALSE(manager.dismiss(id));
    EXPECT_EQ(manager.visibleCount(),0);
}

TEST(ToastManager, RendersAndDismissesWithMouse)
{
    ToastManager manager;
    (void)manager.enqueue({.message = "Success",.severity = ToastSeverity::Success});
    const Size screen {.width = 30,.height = 10};
    EXPECT_EQ(manager.occupiedHeight(screen),4);
    ScreenBuffer buffer(screen);
    Painter painter(buffer);
    manager.paint(painter,screen,Theme::dark());
    EXPECT_NE(buffer.at(0,0).grapheme,"╭");
    EXPECT_TRUE(manager.handleMouse({.position = {.x = 20,.y = 8},
        .button = MouseButton::Left,.action = MouseAction::Press},screen));
    EXPECT_EQ(manager.visibleCount(),0);
}

TEST(ToastManager, NotifiesApplicationWhenQueueChanges)
{
    Application app(std::make_unique<HeadlessTerminalBackend>(Size{.width = 40,.height = 15}));
    app.render([] { return Typography({.text = "Ready"}); });
    app.renderOnce();
    EXPECT_FALSE(app.needsRender());
    const auto id = app.toasts().enqueue({.message = "Saved"});
    EXPECT_TRUE(app.needsRender());
    app.renderOnce();
    EXPECT_FALSE(app.needsRender());
    EXPECT_TRUE(app.toasts().dismiss(id));
    EXPECT_TRUE(app.needsRender());
}
