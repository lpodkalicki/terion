/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <terion/terion.h>
#include <gtest/gtest.h>

using namespace terion;

TEST(UnicodeRenderer, Behavior)
{
    ScreenBuffer screen({.width = 12,.height = 3});
    Painter painter(screen);
    painter.text(0,0,"Łódź");
    EXPECT_EQ(screen.at(0,0).grapheme,"Ł");
    EXPECT_EQ(screen.at(3,0).grapheme,"ź");
    painter.text(0,1,"界");
    EXPECT_EQ(screen.at(0,1).width,2);
    EXPECT_EQ(screen.at(1,1).width,0);
    painter.put(1,1,"A");
    EXPECT_EQ(screen.at(0,1).grapheme," ");
    EXPECT_EQ(screen.at(1,1).grapheme,"A");
    painter.text(3,1,"e\xCC\x81");
    EXPECT_EQ(screen.at(3,1).grapheme,"e\xCC\x81");
    painter.text(11,1,"界");
    EXPECT_EQ(screen.at(11,1).grapheme," ");
    painter.text(11,1,"A\xCC\x81");
    EXPECT_EQ(screen.at(11,1).grapheme,"A\xCC\x81");
    painter.styleAt(1,0,TextStyle{.foreground = Color::rgb(1,2,3),.background = Color::rgb(4,5,6)});
    EXPECT_EQ(screen.at(1,0).grapheme,"ó");
    EXPECT_EQ(screen.at(1,0).style.background,Color::rgb(4,5,6));
    painter.text(4,2,"界");
    painter.styleAt(5,2,TextStyle{.foreground = Color::rgb(3,2,1),.background = Color::rgb(6,5,4)});
    EXPECT_EQ(screen.at(4,2).grapheme,"界");
    EXPECT_EQ(screen.at(4,2).style.background,Color::rgb(6,5,4));
    EXPECT_EQ(screen.at(5,2).style.background,Color::rgb(6,5,4));
    EXPECT_TRUE(isValidUtf8("Łódź"));
    EXPECT_TRUE(!isValidUtf8("\xFF"));
    HeadlessTerminalBackend terminal({12,3});
    terminal.resize({.width = 14,.height = 4});
    const auto resized = terminal.pollEvent();
    ASSERT_TRUE(resized.has_value());
    EXPECT_EQ(resized->key,Key::Resize);
    EXPECT_EQ(resized->size,(Size{.width = 14,.height = 4}));
    terminal.resize({.width = 12,.height = 3});
    EXPECT_EQ(terminal.pollEvent()->key,Key::Resize);
    DifferentialRenderer renderer;
    renderer.render(screen,terminal);
    EXPECT_TRUE(!terminal.output().empty());
    terminal.clearOutput();
    renderer.render(screen,terminal);
    EXPECT_TRUE(terminal.output().empty());
    painter.put(2,1,"B");
    renderer.render(screen,terminal);
    EXPECT_NE(terminal.output().find("B"),std::string::npos);
    terminal.clearOutput();
    painter.text(5,1,"XY");
    renderer.render(screen,terminal);
    EXPECT_NE(terminal.output().find("XY"),std::string::npos);
    EXPECT_EQ(terminal.output().find("\033[2J"),std::string::npos);
    EXPECT_EQ(terminal.output().find("\033[2;7H"),std::string::npos);
    terminal.clearOutput();
    painter.text(7,1,"界");
    renderer.render(screen,terminal);
    terminal.clearOutput();
    painter.put(8,1,"Z");
    renderer.render(screen,terminal);
    EXPECT_NE(terminal.output().find(" Z"),std::string::npos);
    EXPECT_EQ(screen.at(7,1).grapheme," ");
    EXPECT_EQ(screen.at(8,1).grapheme,"Z");
}
