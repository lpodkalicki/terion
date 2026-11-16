/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

// Verify that the three focused umbrella headers are independently usable.
#include <terion/core.h>
#include <terion/components.h>
#include <terion/render.h>
#include <terion/terion.h>
#include <gtest/gtest.h>
#include <type_traits>

using namespace terion;

static_assert(std::is_same_v<decltype(Button::Props{}.size),ControlSize>);
static_assert(std::is_same_v<decltype(TextField::Props{}.variant),InputVariant>);
static_assert(std::is_same_v<decltype(Table::Props{}.selected_key),std::string>);
static_assert(std::is_same_v<decltype(VirtualTable::Props{}.on_selection_key_change),
    std::function<void(std::string)>>);
static_assert(std::is_same_v<decltype(VirtualList::Props{}.on_selection_change),
    std::function<void(int)>>);

TEST(PublicApiHeaders, CanonicalPropsAndRenderHelpers)
{
    const auto md = MarkdownView::render("# Headline");
    const auto diff = DiffView::render("@@ -1 +1 @@\n-old\n+new",DiffViewMode::Unified);
    const auto braille = Chart::render(std::vector<double>{1.0,2.0},8,2);
    EXPECT_EQ(md.size(),1u);
    EXPECT_EQ(diff.size(),3u);
    EXPECT_EQ(braille.size(),2u);
}
