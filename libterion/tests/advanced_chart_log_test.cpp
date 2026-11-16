/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <gtest/gtest.h>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <terion/terion.h>

using namespace terion;

TEST(AdvancedLogView, SequenceRemainsStableAcrossEvictionAndClear)
{
    LogBuffer buffer(2);
    buffer.append({.message="a"});
    const auto first=buffer.at(0).sequence;
    buffer.append({.message="b"});
    const auto second=buffer.at(1).sequence;
    buffer.append({.message="c"});
    EXPECT_EQ(buffer.size(),2u);
    EXPECT_EQ(buffer.at(0).sequence,second);
    EXPECT_GT(buffer.at(1).sequence,second);
    EXPECT_GT(buffer.at(0).sequence,first);
    const auto last=buffer.at(1).sequence;
    buffer.clear();
    buffer.append({.message="d"});
    EXPECT_GT(buffer.at(0).sequence,last);
}

TEST(AdvancedLogView, SnapshotRemainsUnchangedWhileSourceMutates)
{
    LogBuffer buffer(2);
    buffer.append({.message="first"});
    auto snapshot=buffer.snapshot();
    buffer.append({.message="second"});
    buffer.append({.message="third"});
    EXPECT_EQ(snapshot->size(),1u);
    EXPECT_EQ(snapshot->at(0).message,"first");
    EXPECT_EQ(buffer.size(),2u);
}

TEST(AdvancedLogView, CacheReusesIndicesAndInvalidatesOnAppendOrFilter)
{
    auto buffer=std::make_shared<LogBuffer>(3);
    buffer->append({.level=LogLevel::Info,.message="fine"});
    buffer->append({.level=LogLevel::Warning,.message="disk"});
    LogFilterCache cache;
    const LogFilter warnings{LogLevel::Warning,"",{}};
    const auto a=cache.resolve(buffer,warnings);
    const auto b=cache.resolve(buffer,warnings);
    EXPECT_EQ(a.get(),b.get());
    ASSERT_EQ(a->size(),1u);
    buffer->append({.level=LogLevel::Error,.message="disk failure"});
    const auto c=cache.resolve(buffer,warnings);
    EXPECT_NE(c.get(),a.get());
    ASSERT_EQ(c->size(),2u);
    const auto only_errors=cache.resolve(buffer,{LogLevel::Error,"",{}});
    ASSERT_EQ(only_errors->size(),1u);
    buffer->append({.level=LogLevel::Error,.message="more failures"});
    EXPECT_EQ(cache.resolve(buffer,warnings)->size(),3u);
    buffer->append({.level=LogLevel::Info,.message="irrelevant"});
    EXPECT_EQ(cache.resolve(buffer,warnings)->size(),2u);
}

TEST(AdvancedLogView, FiltersBySearchAndExplicitSeverity)
{
    LogBuffer buffer;
    buffer.append({.level=LogLevel::Info,.message="service ready"});
    buffer.append({.level=LogLevel::Warning,.message="DISK warn"});
    buffer.append({.level=LogLevel::Error,.message="disk failed"});
    const auto indices=LogView::filtered(buffer,{LogLevel::Trace,"disk",{LogLevel::Error}});
    ASSERT_EQ(indices.size(),1u);
    EXPECT_EQ(buffer.at(indices[0]).message,"disk failed");
    EXPECT_TRUE(LogView::filtered(buffer,{LogLevel::Critical,"",{}}).empty());
}

TEST(AdvancedLogView, TsvExportEscapesNewlinesAndTabs)
{
    LogBuffer buffer;
    buffer.append({.timestamp="12:00\tUTC",.level=LogLevel::Error,.message="failed\nagain"});
    const auto exported=LogView::exportTsv(buffer);
    EXPECT_NE(exported.find("timestamp\tlevel\tmessage\n"),std::string::npos);
    EXPECT_NE(exported.find("12:00 UTC\tERROR\tfailed again\n"),std::string::npos);
}

TEST(AdvancedLogView, SearchHighlightUsesUnderlineAndBackground)
{
    StyledText result;
    log_detail::appendHighlighted(result,"prefix DISK suffix",ColorVariant::Default,false,"disk");
    ASSERT_EQ(result.size(),3u);
    EXPECT_EQ(result[1].text,"DISK");
    EXPECT_TRUE(result[1].underline);
    ASSERT_TRUE(result[1].background.has_value());
    EXPECT_EQ(*result[1].background,ColorVariant::Warning);
}

TEST(AdvancedChart, ThresholdAndGridAppearWithoutChangingSeriesGeometry)
{
    const std::vector<Chart::Series> data={{.name="CPU",.values={0,20,40,60,100},
        .color=ColorVariant::Success}};
    const auto plain=Chart::renderAdvanced(data,60,12,Chart::Variant::Line,
        true,true,true,false,true,0,1,0,false);
    const auto advanced=Chart::renderAdvanced(data,60,12,Chart::Variant::Line,
        true,true,true,false,true,0,1,0,false,{{75,"alarm",ColorVariant::Warning}},true,"%");
    ASSERT_EQ(plain.size(),advanced.size());
    EXPECT_FALSE(advanced[0].empty());
    bool has_warning=false;
    bool has_legend=false;
    for (const auto& row : advanced)
        for (const auto& span : row)
        {
            if (span.color == ColorVariant::Warning) has_warning=true;
            if (span.text.find("alarm") != std::string::npos) has_legend=true;
        }
    EXPECT_TRUE(has_warning);
    EXPECT_TRUE(has_legend);
}

TEST(AdvancedChart, OutOfRangeThresholdDoesNotRender)
{
    const std::vector<Chart::Series> data={{.name="CPU",.values={0,100},
        .color=ColorVariant::Success}};
    const auto plain=Chart::renderAdvanced(data,50,8,Chart::Variant::Line,
        true,true,false,false,true,0,1,0,false);
    const auto out=Chart::renderAdvanced(data,50,8,Chart::Variant::Line,
        true,true,false,false,true,0,1,0,false,{{1000,"",ColorVariant::Error}},false);
    ASSERT_EQ(plain.size(),out.size());
    for (std::size_t row=0;row<plain.size();++row)
    {
        std::string a,b;
        for (const auto& span : plain[row]) a += span.text;
        for (const auto& span : out[row]) b += span.text;
        EXPECT_EQ(a,b);
    }
}

TEST(AdvancedChart, MinMaxSamplingRetainsSpikeAndOriginalIndices)
{
    std::vector<double> values(1000,2.0);
    values[310]=97.0;
    values[700]=-18.0;
    const auto points=Chart::minMaxSample(values,20);
    EXPECT_LE(points.size(),40u);
    bool high=false,low=false;
    for (const auto& point : points)
    {
        EXPECT_EQ(point.value,values[point.index]);
        if (point.index==310 && point.value==97.0) high=true;
        if (point.index==700 && point.value==-18.0) low=true;
    }
    EXPECT_TRUE(high);
    EXPECT_TRUE(low);
    EXPECT_TRUE(Chart::minMaxSample(values,0).empty());
}
