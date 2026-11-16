/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <terion/terion.h>
#include <terion/emoji_compat.h>
#include <gtest/gtest.h>

using namespace terion;

namespace {

class LegacyEmojiBackend final : public TerminalBackend
{
public:
    explicit LegacyEmojiBackend(bool compatibility) : compatibility_(compatibility) {}
    Size size() const override { return {.width = 32,.height = 3}; }
    void write(std::string_view value) override { output_.append(value); }
    void flush() override {}
    std::optional<Event> pollEvent() override { return std::nullopt; }
    TerminalCapabilities capabilities() const override
    {
        return {.emoji_compatibility = compatibility_};
    }
    const std::string& output() const { return output_; }
private:
    bool compatibility_;
    std::string output_;
};

} /* End of anonymous namespace */

TEST(EmojiCompat, RendersLegacyEmojiWithinTheLogicalCellWidth)
{
    EXPECT_EQ(compatibleTerminalGrapheme("👨‍👩‍👧",2),"👨");
    EXPECT_EQ(compatibleTerminalGrapheme("👍🏽",2),"👍");
    EXPECT_EQ(compatibleTerminalGrapheme("❤️",2),"❤ ");
    EXPECT_EQ(compatibleTerminalGrapheme("🇵🇱",2),"🇵🇱");
    EXPECT_EQ(compatibleTerminalGrapheme("1️⃣",2),"1️⃣");
    EXPECT_EQ(compatibleTerminalGrapheme("e\u0301",1),"e\u0301");
    EXPECT_EQ(compatibleTerminalGrapheme("│",1),"│");
    EXPECT_EQ(compatibleTerminalGrapheme("",1),"");
}

TEST(EmojiCompat, KeepsScreenBufferAndClipboardContentsUnchanged)
{
    ScreenBuffer buffer({.width = 32,.height = 3});
    Painter painter(buffer);
    painter.text(0,0,"👨‍👩‍👧 👍🏽 ❤️ │");
    EXPECT_EQ(buffer.at(0,0).grapheme,"👨‍👩‍👧");
    EXPECT_EQ(buffer.at(3,0).grapheme,"👍🏽");
    EXPECT_EQ(buffer.at(6,0).grapheme,"❤️");
    EXPECT_EQ(buffer.at(9,0).grapheme,"│");
    Clipboard::set("👨‍👩‍👧 👍🏽 ❤️ │");
    EXPECT_EQ(Clipboard::get(),"👨‍👩‍👧 👍🏽 ❤️ │");

    LegacyEmojiBackend legacy(true);
    DifferentialRenderer renderer;
    renderer.render(buffer,legacy);
    EXPECT_EQ(legacy.output().find("👨‍👩‍👧"),std::string::npos);
    EXPECT_EQ(legacy.output().find("👍🏽"),std::string::npos);
    EXPECT_EQ(legacy.output().find("❤️"),std::string::npos);
    EXPECT_NE(legacy.output().find("👨"),std::string::npos);
    EXPECT_NE(legacy.output().find("👍"),std::string::npos);
    EXPECT_NE(legacy.output().find("❤ "),std::string::npos);
    EXPECT_NE(legacy.output().find("│"),std::string::npos);

    LegacyEmojiBackend modern(false);
    DifferentialRenderer other_renderer;
    other_renderer.render(buffer,modern);
    EXPECT_NE(modern.output().find("👨‍👩‍👧"),std::string::npos);
    EXPECT_NE(modern.output().find("👍🏽"),std::string::npos);
    EXPECT_NE(modern.output().find("❤️"),std::string::npos);
}

TEST(EmojiCompat, TerminalModeCanBeDetectedOrOverridden)
{
    EXPECT_TRUE(AnsiTerminalBackend::detectCapabilities("Apple_Terminal").emoji_compatibility);
    EXPECT_FALSE(AnsiTerminalBackend::detectCapabilities("vscode").emoji_compatibility);
    EXPECT_TRUE(AnsiTerminalBackend::detectCapabilities("vscode","","1").emoji_compatibility);
    EXPECT_FALSE(AnsiTerminalBackend::detectCapabilities("Apple_Terminal","","0").emoji_compatibility);
}
