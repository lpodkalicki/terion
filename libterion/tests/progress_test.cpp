/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <gtest/gtest.h>
#include <terion/terion.h>

using namespace terion;

namespace {

std::string progressGlyphs(const Progress::Props& props, bool unicode = true)
{
    Reconciler reconciler;
    reconciler.update(Progress(props));
    const Size screen {.width = 20,.height = 1};
    reconciler.root()->layout(Constraints{.minimum = screen,.maximum = screen});
    ScreenBuffer buffer(screen);
    Painter painter(buffer);
    const Theme theme = Theme::dark();
    reconciler.root()->paint(painter,{},{},std::nullopt,&theme,unicode);
    std::string result;
    for (int x = 0; x < screen.width; ++x) result += buffer.at(x,0).grapheme;
    return result;
}

} /* End of anonymous namespace */

TEST(Progress, RendersDeterminateAndClampsValues)
{
    EXPECT_EQ(progressGlyphs({.value = -1.0,.variant = ProgressVariant::Ascii}),
        "--------------------");
    EXPECT_EQ(progressGlyphs({.value = 0.5,.variant = ProgressVariant::Ascii}),
        "##########----------");
    EXPECT_EQ(progressGlyphs({.value = 2.0,.variant = ProgressVariant::Ascii}),
        "####################");
}

TEST(Progress, DisplaysOptionalPercentage)
{
    EXPECT_EQ(progressGlyphs({.value = 0.42,.variant = ProgressVariant::Ascii,
        .showPercentage = true}),"######--------- 42% ");
}

TEST(Progress, UsesAsciiWhenUnicodeUnavailable)
{
    EXPECT_EQ(progressGlyphs({.value = 0.5},false),"##########----------");
}

TEST(Progress, DetectsActiveIndeterminateProgress)
{
    Reconciler reconciler;
    reconciler.update(Progress({.indeterminate = true}));
    reconciler.root()->layout(Constraints{.minimum = {.width = 20,.height = 1},
        .maximum = {.width = 20,.height = 1}});
    EXPECT_TRUE(reconciler.root()->hasIndeterminateProgress());
    reconciler.update(Progress({.value = 0.5}));
    EXPECT_FALSE(reconciler.root()->hasIndeterminateProgress());
}
