/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <string>
#include <utility>
#include <vector>
#include <gtest/gtest.h>
#include <terion/terion.h>

using namespace terion;

TEST(StepperAdvanced, AutoStatusesAndCompletedLastStep)
{
    Stepper::Props props{.steps={{"One"},{"Two"},{"Three"}},.active_step=1};
    EXPECT_EQ(Stepper::statusAt(props,0),StepStatus::Completed);
    EXPECT_EQ(Stepper::statusAt(props,1),StepStatus::Active);
    EXPECT_EQ(Stepper::statusAt(props,2),StepStatus::Pending);
    props.active_step=2;
    props.completed=true;
    EXPECT_EQ(Stepper::statusAt(props,2),StepStatus::Completed);
    EXPECT_EQ(Stepper::statusAt(props,3),StepStatus::Pending);
}

TEST(StepperAdvanced, ExplicitStatusesAndVerticalOrientation)
{
    Stepper component({.steps={{.label="Validated",.status=StepStatus::Completed},
        {.label="Failed",.status=StepStatus::Error,.description="Invalid configuration"},
        {.label="Skipped",.status=StepStatus::Skipped}},.active_step=1,
        .orientation=StepperOrientation::Vertical});
    const auto& root=component.props();
    EXPECT_EQ(root.type,Component::Type::Column);
    ASSERT_EQ(root.children.size(),5u);
    EXPECT_EQ(root.children[0].props().textData().color_variant,ColorVariant::Success);
    EXPECT_EQ(root.children[2].props().textData().color_variant,ColorVariant::Error);
    EXPECT_EQ(root.children[4].props().textData().color_variant,ColorVariant::Warning);
    EXPECT_NE(root.children[2].props().textData().text.find("Invalid configuration"),std::string::npos);
}

TEST(StepperAdvanced, ValidationPreventsNavigationWithoutDisablingBackwardSteps)
{
    std::size_t selected=99;
    int validation_calls=0;
    Stepper stepper({.steps={{"One"},{"Two"},{"Three"}},.active_step=1,
        .linear=false,.on_change=[&](std::size_t index) { selected=index; },
        .on_before_change=[&](std::size_t from,std::size_t to) {
            ++validation_calls;
            return to < from;
        }});
    const auto& buttons=stepper.props().children;
    ASSERT_EQ(buttons.size(),5u);
    ASSERT_TRUE(static_cast<bool>(buttons[4].props().actionData().on_click));
    buttons[4].props().actionData().on_click();
    EXPECT_EQ(selected,99u);
    buttons[0].props().actionData().on_click();
    EXPECT_EQ(selected,0u);
    EXPECT_EQ(validation_calls,2);
    EXPECT_TRUE(buttons[2].props().disabled);
}

TEST(AccordionAdvanced, ExpandAllSkipsDisabledItems)
{
    std::vector<Accordion::Item> items{
        {.id="a",.title="First"},
        {.id="b",.title="Disabled",.disabled=true},
        {.id="c",.title="Third"}};
    EXPECT_EQ(Accordion::expandAll(items),(std::vector<std::string>{"a","c"}));
    EXPECT_TRUE(Accordion::collapseAll().empty());
}

TEST(AccordionAdvanced, KeepMountedRetainsRenderNodeOnCollapse)
{
    const auto render=[](const bool expanded) {
        return Accordion({.items={{.id="settings",.title="Settings",.children={
            TextField({.value="draft",.key="editor"})}}},
            .expanded=expanded?std::vector<std::string>{"settings"}:std::vector<std::string>{},
            .keep_mounted=true});
    };
    Reconciler tree;
    tree.update(render(true));
    auto* section=tree.root()->children()[0].get();
    ASSERT_EQ(section->children().size(),2u);
    auto* panel=section->children()[1].get();
    const auto panel_id=panel->id();
    const auto child_id=panel->children()[0]->children()[0]->id();
    tree.update(render(false));
    section=tree.root()->children()[0].get();
    ASSERT_EQ(section->children().size(),2u);
    panel=section->children()[1].get();
    EXPECT_EQ(panel->id(),panel_id);
    EXPECT_EQ(panel->children()[0]->children()[0]->id(),child_id);
    const auto measured=panel->measure({.maximum={.width=40,.height=20}});
    EXPECT_EQ(measured.height,0);
    tree.update(render(true));
    section=tree.root()->children()[0].get();
    EXPECT_EQ(section->children()[1]->children()[0]->children()[0]->id(),child_id);
}

TEST(AccordionAdvanced, DefaultBehaviorStillUnmountsContent)
{
    Accordion open({.items={{.id="a",.title="A",.children={Typography({.text="body"})}}},
        .expanded={"a"}});
    Accordion closed({.items={{.id="a",.title="A",.children={Typography({.text="body"})}}}});
    EXPECT_EQ(open.props().children[0].props().children.size(),2u);
    EXPECT_EQ(closed.props().children[0].props().children.size(),1u);
}

TEST(BreadcrumbsAdvanced, CollapsesHiddenItemsIntoNavigableMenu)
{
    std::string selected;
    Breadcrumbs breadcrumbs({.items={{"Home","/"},{"Workspace","/w"},
        {"Projects","/w/p",true},{"Repository","/w/p/r"},{"Components","/w/p/r/c"},
        {"Stepper","/w/p/r/c/s"}},
        .on_navigate=[&](std::string value) { selected=std::move(value); },
        .max_visible_items=4});
    const auto& children=breadcrumbs.props().children;
    ASSERT_EQ(children.size(),7u);
    EXPECT_EQ(children[2].props().type,Component::Type::DropdownMenu);
    const auto& menu=children[2].props();
    ASSERT_EQ(menu.choiceData().select_options.size(),3u);
    EXPECT_EQ(menu.choiceData().select_options[0].label,"Workspace");
    EXPECT_TRUE(menu.choiceData().select_options[1].disabled);
    ASSERT_TRUE(static_cast<bool>(menu.actionData().on_change));
    menu.actionData().on_change("/w");
    EXPECT_EQ(selected,"/w");
    EXPECT_EQ(children[4].props().textData().text,"Components");
    EXPECT_EQ(children[6].props().textData().text,"Stepper");
}

TEST(BreadcrumbsAdvanced, GraphemeAwareAbbreviationPreservesEmoji)
{
    const auto shortened=Breadcrumbs::abbreviate("🇵🇱 Documents",3);
    EXPECT_EQ(shortened,"🇵🇱…");
    EXPECT_EQ(displayWidth(shortened),3);
    EXPECT_EQ(Breadcrumbs::abbreviate("Very long label",1),"…");
    EXPECT_EQ(Breadcrumbs::abbreviate("Short",0),"Short");
    EXPECT_EQ(Breadcrumbs::abbreviate("Short",8),"Short");
}

TEST(BreadcrumbsAdvanced, DoesNotCollapseByDefault)
{
    Breadcrumbs breadcrumbs({.items={{"A","a"},{"B","b"},{"C","c"},{"D","d"}}});
    EXPECT_EQ(breadcrumbs.props().children.size(),7u);
    for (const auto& entry : breadcrumbs.props().children)
        EXPECT_NE(entry.props().type,Component::Type::DropdownMenu);
}
