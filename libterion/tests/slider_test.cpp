/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <gtest/gtest.h>
#include <terion/terion.h>

using namespace terion;

TEST(Slider, HandlesKeyboardAndBounds)
{
    double selected = -1;
    Reconciler slider;
    slider.update(Slider({.value = 50,.min = 0,.max = 100,.step = 10,
        .onChange = [&](double value) { selected = value; }}));
    auto* node = slider.root();
    EXPECT_TRUE(node->handleSliderKey({.key = Key::Right}));
    EXPECT_DOUBLE_EQ(selected,60);
    EXPECT_TRUE(node->handleSliderKey({.key = Key::Home}));
    EXPECT_DOUBLE_EQ(selected,0);
    EXPECT_TRUE(node->handleSliderKey({.key = Key::End}));
    EXPECT_DOUBLE_EQ(selected,100);
    EXPECT_FALSE(node->handleSliderKey({.key = Key::Enter}));
}

TEST(Slider, SnapsMousePositionToStep)
{
    double selected = -1;
    Reconciler slider;
    slider.update(Slider({.value = 0,.min = 0,.max = 100,.step = 10,
        .showValue = false,.onChange = [&](double value) { selected = value; },
        .layout = {.width = 11}}));
    slider.root()->layout({.maximum = {.width = 11,.height = 1}});
    EXPECT_TRUE(slider.root()->handleSliderMouse({.x = 5,.y = 0}));
    EXPECT_DOUBLE_EQ(selected,50);
}

TEST(Slider, DisabledSliderDoesNotHandleInput)
{
    Reconciler slider;
    slider.update(Slider({.value = 50,.disabled = true,.onChange = [](double) {}}));
    EXPECT_FALSE(slider.root()->sliderInteractive());
    EXPECT_FALSE(slider.root()->handleSliderKey({.key = Key::Right}));
}
