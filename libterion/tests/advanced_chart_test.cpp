/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <cmath>
#include <limits>
#include <gtest/gtest.h>
#include <terion/terion.h>

using namespace terion;

TEST(AdvancedChart, DrawsMultipleSeriesWithDistinctStyles)
{
    const auto rows = Chart::renderAdvanced({
        {.name = "LOW",.values = {0,1,0},.color = ColorVariant::Success},
        {.name = "HIGH",.values = {3,2,3},.color = ColorVariant::Warning}
    },40,9);
    ASSERT_EQ(rows.size(),9u);
    bool green = false;
    bool yellow = false;
    for (const auto& row : rows)
        for (const auto& span : row)
        {
            green |= span.color == ColorVariant::Success;
            yellow |= span.color == ColorVariant::Warning;
        }
    EXPECT_TRUE(green);
    EXPECT_TRUE(yellow);
}

TEST(AdvancedChart, HandlesMissingValuesAndInvalidBounds)
{
    const auto nan = std::numeric_limits<double>::quiet_NaN();
    const auto rows = Chart::renderAdvanced({
        {.name = "gaps",.values = {nan,1,nan,2},.color = ColorVariant::Primary}
    },20,8,Chart::Variant::Line,true,true,true,false,false,4,4);
    EXPECT_EQ(rows.size(),8u);
    EXPECT_TRUE(Chart::renderAdvanced({},0,6).empty());
}

TEST(AdvancedChart, PreservesLegacyApi)
{
    EXPECT_EQ(Chart::render({1,2,3},10,4).size(),4u);
    const auto rows = Chart::renderAdvanced({},20,7);
    EXPECT_EQ(rows.size(),7u);
}
