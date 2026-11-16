/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <gtest/gtest.h>
#include <terion/terion.h>

using namespace terion;

TEST(UnicodeAlignment, ResizableSplitDividerKeepsItsColumnAfterUnicodeText)
{
    Reconciler tree;
    tree.update(ResizableSplit({.children={
        Column({.children={Typography({.text="Zażółć gęślą jaźń 🇵🇱 👨‍👩‍👧 👍🏽"}),
            MarkdownView({.markdown="## Unicode\n🇵🇱 · 👨‍👩‍👧 · 👍🏽",.layout={.width=37,.height=9}})},
            .layout={.width=40,.height=12,.padding=1}}),
        Column({.children={Typography({.text="Diff 🇵🇱"}),
            DiffView({.diff="@@ -1 +1 @@\n-old 👨‍👩‍👧\n+new 🇵🇱 👍🏽\n",
                .layout={.width=37,.height=9},.mode=DiffViewMode::SideBySide})},
            .layout={.width=40,.height=12,.padding=1}})},
        .position=40,.layout={.width=81,.height=12}}));
    tree.root()->layout({.maximum={.width=81,.height=12}});
    ScreenBuffer screen({.width=81,.height=12});
    Painter painter(screen);
    tree.root()->paint(painter);
    const Rect divider=tree.root()->split_handle();
    ASSERT_EQ(divider.width,1);
    for (int row=divider.y;row<divider.y+divider.height;++row)
    {
        SCOPED_TRACE(row);
        EXPECT_EQ(screen.at(divider.x,row).grapheme,"│");
        EXPECT_EQ(screen.at(divider.x,row).width,1);
    }
    HeadlessTerminalBackend backend({81,12});
    DifferentialRenderer renderer;
    renderer.render(screen,backend);
    const std::string output=backend.output();
    for (int row=divider.y;row<divider.y+divider.height;++row)
        EXPECT_NE(output.find("\033["+std::to_string(row+1)+";"+
            std::to_string(divider.x+1)+"H"),std::string::npos);
}
