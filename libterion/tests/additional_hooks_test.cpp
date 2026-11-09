/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <terion/terion.h>
#include <gtest/gtest.h>

using namespace terion;

namespace {

int effect_starts = 0;
int effect_cleanups = 0;
int memo_calculations = 0;
std::shared_ptr<int> last_ref;

const auto HooksPanel = component([](int value) {
    auto ref = useRef(5);
    last_ref = ref;
    auto doubled = useMemo([value] {
        ++memo_calculations;
        return value * 2;
    },value);
    auto callback = useCallback([value] { return value; },value);
    useEffect([value] {
        ++effect_starts;
        return [value] { (void)value; ++effect_cleanups; };
    },value);
    return Typography({.text = std::to_string(doubled + callback() + *ref)});
});

} /* End of anonymous namespace */

TEST(AdditionalHooks, MemoRefCallbackAndEffectLifecycle)
{
    effect_starts = effect_cleanups = memo_calculations = 0;
    last_ref.reset();
    Reconciler reconciler;
    reconciler.update(HooksPanel(2).keyed("panel"));
    EXPECT_EQ(reconciler.root()->text(),"11");
    EXPECT_EQ(effect_starts,0);
    reconciler.commitEffects();
    EXPECT_EQ(effect_starts,1);

    *last_ref = 9;
    reconciler.update(HooksPanel(2).keyed("panel"));
    EXPECT_EQ(reconciler.root()->text(),"15");
    EXPECT_EQ(memo_calculations,1);
    reconciler.commitEffects();
    EXPECT_EQ(effect_starts,1);
    EXPECT_EQ(effect_cleanups,0);

    reconciler.update(HooksPanel(3).keyed("panel"));
    EXPECT_EQ(reconciler.root()->text(),"18");
    reconciler.commitEffects();
    EXPECT_EQ(memo_calculations,2);
    EXPECT_EQ(effect_starts,2);
    EXPECT_EQ(effect_cleanups,1);

    reconciler.update(Typography({.text = "Unmount"}));
    reconciler.commitEffects();
    EXPECT_EQ(effect_cleanups,2);
}

TEST(AdditionalHooks, RejectsHooksOutsideMountedComponent)
{
    EXPECT_THROW((void)useRef(1),std::logic_error);
    EXPECT_THROW((void)useMemo([] { return 1; }),std::logic_error);
    EXPECT_THROW(useEffect([] {}),std::logic_error);
}
