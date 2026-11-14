/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <gtest/gtest.h>
#include <terion/terion.h>

using namespace terion;

TEST(Accordion, ToggleSupportsSingleAndMultipleModes)
{
    EXPECT_EQ(Accordion::toggle({},"first",false),(std::vector<std::string>{"first"}));
    EXPECT_EQ(Accordion::toggle({"first"},"first",false),(std::vector<std::string>{}));
    EXPECT_EQ(Accordion::toggle({"first"},"second",false),(std::vector<std::string>{"second"}));
    EXPECT_EQ(Accordion::toggle({"first"},"second",true),
        (std::vector<std::string>{"first","second"}));
    EXPECT_EQ(Accordion::toggle({"first","second"},"first",true),
        (std::vector<std::string>{"second"}));
}

TEST(CompositeComponents, RenderWithoutCustomRendererTypes)
{
    Reconciler reconciler;
    reconciler.update(Column({.children={
        Stepper({.steps={{"First"},{"Second"}},.active_step=1}),
        Accordion({.items={{.id="a",.title="Section",.children={Typography({.text="Body"})}}},
            .expanded={"a"}}),
        Breadcrumbs({.items={{"Root","root"},{"Leaf","leaf"}}})
    }}));
    ASSERT_NE(reconciler.root(),nullptr);
    reconciler.root()->layout({.maximum={.width=100,.height=20}});
    ScreenBuffer screen({.width=100,.height=20});
    Painter painter(screen);
    reconciler.root()->paint(painter);
    EXPECT_NE(screen.at(0,0).grapheme,"");
}

TEST(CompositeComponents, HandlesEmptyContent)
{
    Reconciler reconciler;
    reconciler.update(Column({.children={Stepper({}),Accordion({}),Breadcrumbs({})}}));
    ASSERT_NE(reconciler.root(),nullptr);
}
