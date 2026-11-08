/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <terion/terion.h>
#include <gtest/gtest.h>

using namespace terion;

TEST(InputDecoder, DecodesEscapeKeysAndUnicodeInput)
{
    InputDecoder decoder;
    decoder.feed("\033");
    EXPECT_TRUE(!decoder.nextEvent());
    EXPECT_EQ(decoder.nextEvent(true)->key,Key::Escape);
    decoder.feed("\033[");
    EXPECT_TRUE(!decoder.nextEvent());
    decoder.feed("A");
    EXPECT_EQ(decoder.nextEvent()->key,Key::Up);
    decoder.feed("\033[3~\033OD");
    EXPECT_EQ(decoder.nextEvent()->key,Key::Delete);
    EXPECT_EQ(decoder.nextEvent()->key,Key::Left);
    decoder.feed("\xC5");
    EXPECT_TRUE(!decoder.nextEvent());
    decoder.feed("\x81");
    const auto letter = decoder.nextEvent();
    ASSERT_TRUE(letter.has_value());
    EXPECT_EQ(letter->key,Key::Character);
    EXPECT_EQ(letter->character,U'Ł');
    decoder.feed("\t\r");
    EXPECT_EQ(decoder.nextEvent()->key,Key::Tab);
    EXPECT_EQ(decoder.nextEvent()->key,Key::Enter);
    decoder.feed("\xE2");
    EXPECT_TRUE(!decoder.nextEvent());
    EXPECT_EQ(decoder.nextEvent(true)->character,0xFFFD);
    decoder.feed("\033[1;5C");
    EXPECT_EQ(decoder.nextEvent()->key,Key::Right);
    decoder.feed("\033[99~");
    EXPECT_EQ(decoder.nextEvent()->key,Key::Unknown);
    EXPECT_TRUE(!decoder.nextEvent());
}

TEST(InputDecoder, DecodesMousePressReleaseAndWheel)
{
    InputDecoder decoder;
    decoder.feed("\033[<0;2;");
    EXPECT_TRUE(!decoder.nextEvent());
    decoder.feed("3M");
    const auto mouse_press = decoder.nextEvent();
    ASSERT_TRUE(mouse_press.has_value());
    EXPECT_EQ(mouse_press->key,Key::Mouse);
    EXPECT_EQ(mouse_press->mouse.action,MouseAction::Press);
    EXPECT_EQ(mouse_press->mouse.button,MouseButton::Left);
    EXPECT_EQ(mouse_press->mouse.position,(Point{.x = 1,.y = 2}));
    decoder.feed("\033[<0;2;3m\033[<64;5;6M");
    EXPECT_EQ(decoder.nextEvent()->mouse.action,MouseAction::Release);
    const auto wheel = decoder.nextEvent();
    ASSERT_TRUE(wheel.has_value());
    EXPECT_EQ(wheel->mouse.action,MouseAction::Scroll);
    EXPECT_EQ(wheel->mouse.scrollY,1);
    decoder.feed("\033[<32;3;4M");
    EXPECT_EQ(decoder.nextEvent()->mouse.action,MouseAction::Move);

}
