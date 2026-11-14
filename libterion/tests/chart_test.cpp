/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <cmath>
#include <cstddef>
#include <limits>
#include <string>
#include <gtest/gtest.h>
#include <terion/terion.h>
#include <terion/chart_buffer.h>

using namespace terion;

namespace {

void expectBrailleRow(const std::string& row,std::size_t columns)
{
    // U+2800..U+28FF uses exactly three UTF-8 bytes per terminal cell.
    ASSERT_EQ(row.size(),columns*3u);
    for (std::size_t i=0;i<row.size();i+=3)
    {
        EXPECT_EQ(static_cast<unsigned char>(row[i]),0xe2u);
        EXPECT_GE(static_cast<unsigned char>(row[i+1]),0xa0u);
        EXPECT_LE(static_cast<unsigned char>(row[i+1]),0xa3u);
        EXPECT_GE(static_cast<unsigned char>(row[i+2]),0x80u);
        EXPECT_LE(static_cast<unsigned char>(row[i+2]),0xbfu);
    }
}

} /* End of anonymous namespace */

TEST(Sparkline, RendersAtRequestedWidth)
{
    EXPECT_EQ(Sparkline::render({1,2,3},4,Sparkline::Variant::Ascii).size(),4u);
    EXPECT_EQ(Sparkline::render({1,2,3},0),"");
    EXPECT_EQ(Sparkline::render({1,2,3},3,Sparkline::Variant::Ascii),".+@");
}

TEST(Sparkline, HandlesMissingAndConstantValues)
{
    const auto missing = std::numeric_limits<double>::quiet_NaN();
    EXPECT_EQ(Sparkline::render({missing,missing},3,Sparkline::Variant::Ascii),"   ");
    EXPECT_EQ(Sparkline::render({5,5,5},3,Sparkline::Variant::Ascii),"+++");
}

TEST(Chart, HasBrailleResolutionAndFiniteBounds)
{
    const auto rows = Chart::render({0,1,0},10,4);
    ASSERT_EQ(rows.size(),4u);
    for (const auto& row : rows) expectBrailleRow(row,10u);
    EXPECT_NE(rows[0],rows[3]);
    EXPECT_TRUE(Chart::render({},0,3).empty());
}

TEST(Chart, HandlesNaNAndModes)
{
    const auto nan = std::numeric_limits<double>::quiet_NaN();
    for (const auto variant : {Chart::Variant::Line,Chart::Variant::Scatter,Chart::Variant::Bars})
    {
        const auto rows = Chart::render({1,nan,3,-2},12,5,variant,true);
        ASSERT_EQ(rows.size(),5u);
        for (const auto& row : rows) expectBrailleRow(row,12u);
    }
}

TEST(Chart, ComponentsPaintThroughVirtualList)
{
    Reconciler reconciler;
    reconciler.update(Chart({.values = {0,1,0},.layout = {.width = 12,.height = 4}}));
    reconciler.root()->layout({.maximum = {.width = 12,.height = 4}});
    ScreenBuffer screen({.width = 12,.height = 4});
    Painter painter(screen);
    reconciler.root()->paint(painter);
    EXPECT_NE(screen.at(0,0).grapheme,"");
}

TEST(ChartInteractive, WindowAndClamping)
{
    const std::vector<Chart::Series> series = {{"CPU",{1,2,3,4},ColorVariant::Success},
        {"RAM",{5,6},ColorVariant::Info}};
    EXPECT_EQ(Chart::sampleCount(series),4u);
    EXPECT_EQ((Chart::clampView(4,3,3)),(std::pair<std::size_t,std::size_t>{1,3}));
    const auto clipped = Chart::window(series,1,2);
    ASSERT_EQ(clipped.size(),2u);
    EXPECT_EQ(clipped[0].values,(std::vector<double>{2,3}));
    EXPECT_EQ(clipped[1].values,(std::vector<double>{6}));
    EXPECT_EQ(Chart::clampView(0,10,4).second,0u);
}

TEST(ChartInteractive, ZoomPanStayWithinBounds)
{
    const auto zoomed = Chart::zoom(100,20,40,1);
    EXPECT_EQ(zoomed.second,20u);
    EXPECT_LE(zoomed.first+zoomed.second,100u);
    const auto panned = Chart::pan(100,95,10,1);
    EXPECT_EQ(panned.first,90u);
    EXPECT_EQ(panned.second,10u);
    EXPECT_EQ(Chart::zoom(0,0,0,1).second,0u);
}

TEST(ChartInteractive, TooltipReportsMissingValues)
{
    const auto nan = std::numeric_limits<double>::quiet_NaN();
    const std::vector<Chart::Series> series = {{"CPU",{1.25,nan},ColorVariant::Success},
        {"RAM",{3.5},ColorVariant::Info}};
    EXPECT_NE(Chart::tooltip(series,0).find("CPU=1.25"),std::string::npos);
    EXPECT_NE(Chart::tooltip(series,1).find("CPU=n/a"),std::string::npos);
    EXPECT_NE(Chart::tooltip(series,1).find("RAM=n/a"),std::string::npos);
}

TEST(ChartRealtime, BoundedRingPreservesStableGlobalIndices)
{
    ChartBuffer buffer(3,2);
    buffer.append({1,10});
    buffer.append({2,20});
    buffer.append({3,30});
    buffer.append({4,40});
    EXPECT_EQ(buffer.size(),3u);
    EXPECT_EQ(buffer.beginIndex(),1u);
    EXPECT_EQ(buffer.endIndex(),4u);
    EXPECT_EQ(buffer.channel(0),(std::vector<double>{2,3,4}));
    EXPECT_EQ(buffer.channel(1),(std::vector<double>{20,30,40}));
}

TEST(ChartRealtime, RingRejectsInvalidInputAndSupportsReset)
{
    EXPECT_THROW(ChartBuffer(0,1),std::invalid_argument);
    ChartBuffer buffer(2,1);
    EXPECT_THROW(buffer.append({1,2}),std::invalid_argument);
    buffer.append({42});
    buffer.clear();
    EXPECT_TRUE(buffer.channel(0).empty());
    EXPECT_EQ(buffer.beginIndex(),0u);
}

TEST(ChartRealtime, IndependentYAxisRendersBothSeries)
{
    const std::vector<Chart::Series> series={
        {"CPU",{10,20,30},ColorVariant::Success},
        {"NET",{1000,2000,3000},ColorVariant::Warning}
    };
    const auto rows=Chart::renderAdvanced(series,40,10,Chart::Variant::Line,
        true,true,true,false,true,0,1,0,true);
    EXPECT_EQ(rows.size(),10u);
    EXPECT_FALSE(rows[0].empty());
}
