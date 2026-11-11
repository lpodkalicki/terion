/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <terion/terion.h>
#include <gtest/gtest.h>
#include <stdexcept>

using namespace terion;

namespace {

const auto Counter = component([] {
    auto [count,set_count] = useState(0);
    return Button({.text = std::to_string(count),
        .on_click = [set_count] { set_count([](int n) { return n+1; }); }});
});

const auto FailingPanel = component([](bool fail) {
    (void)useState(7);
    if (fail) throw std::runtime_error("panel unavailable");
    return Typography({.text = "Working"});
});

Component panel(bool fail)
{
    return ErrorBoundary({.child = FailingPanel(fail),
        .fallback = [](const std::string& error) {
            return Typography({.text = "Error: " + error});
        },.key = "panel"});
}

} /* End of anonymous namespace */

TEST(ErrorBoundary, ShowsFallbackAndRecovers)
{
    Reconciler reconciler;
    reconciler.update(panel(false));
    EXPECT_EQ(reconciler.root()->text(),"Working");

    EXPECT_NO_THROW(reconciler.update(panel(true)));
    EXPECT_EQ(reconciler.root()->text(),"Error: panel unavailable");

    EXPECT_NO_THROW(reconciler.update(panel(false)));
    EXPECT_EQ(reconciler.root()->text(),"Working");
}

TEST(ErrorBoundary, DoesNotAffectSiblingState)
{
    Reconciler reconciler;
    auto view = [](bool fail) {
        return Column({.children = {Counter().keyed("counter"),panel(fail)}});
    };
    reconciler.update(view(false));
    reconciler.root()->children()[0]->onClick()();
    reconciler.update(view(true));
    ASSERT_EQ(reconciler.root()->children().size(),2u);
    EXPECT_EQ(reconciler.root()->children()[0]->text(),"1");
    EXPECT_EQ(reconciler.root()->children()[1]->text(),"Error: panel unavailable");
}

TEST(ErrorBoundary, FallbackFailuresPropagate)
{
    Reconciler reconciler;
    EXPECT_THROW(reconciler.update(ErrorBoundary({.child = FailingPanel(true),
        .fallback = [](const std::string&) -> Component {
            throw std::runtime_error("fallback failed");
        }})),std::runtime_error);
}
