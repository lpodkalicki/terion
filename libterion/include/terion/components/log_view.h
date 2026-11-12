/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#ifndef TERION_COMPONENTS_LOG_VIEW_H
#define TERION_COMPONENTS_LOG_VIEW_H

#include <algorithm>
#include <cstddef>
#include <deque>
#include <functional>
#include <limits>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include <terion/components/virtual_list.h>

namespace terion {

enum class LogLevel { Trace, Debug, Info, Warning, Error, Critical };

struct LogEntry
{
    std::string timestamp = {};
    LogLevel level = LogLevel::Info;
    std::string message = {};
}; /* End of struct LogEntry */

// UI-thread-owned bounded buffer. Append and rendering should run on the same thread.
class LogBuffer
{
public:
    explicit LogBuffer(std::size_t capacity = 10000) : capacity_(capacity) {}

    void append(LogEntry entry)
    {
        if (capacity_ == 0) return;
        if (entries_.size() == capacity_) entries_.pop_front();
        entries_.push_back(std::move(entry));
    }

    void clear() noexcept { entries_.clear(); }
    [[nodiscard]] std::size_t size() const noexcept { return entries_.size(); }
    [[nodiscard]] std::size_t capacity() const noexcept { return capacity_; }
    [[nodiscard]] const LogEntry& at(std::size_t index) const { return entries_.at(index); }

private:
    std::size_t capacity_;
    std::deque<LogEntry> entries_;
}; /* End of class LogBuffer */

class LogView : public Component
{
public:
    struct Props
    {
        std::shared_ptr<const LogBuffer> buffer = {};
        LogLevel min_level = LogLevel::Trace;
        std::string search = {};
        int selected_index = -1;
        bool follow_tail = false;
        std::function<void(int)> on_select = {};
        bool show_timestamp = true;
        bool show_level = true;
        std::string key = {};
        LayoutStyle layout = {};
        bool disabled = false;
        bool show_scrollbar = true;
    }; /* End of struct Props */

    explicit LogView(Props props) : Component(create(std::move(props))) {}

private:
    static std::string levelName(LogLevel level)
    {
        switch (level)
        {
            case LogLevel::Trace: return "TRACE";
            case LogLevel::Debug: return "DEBUG";
            case LogLevel::Info: return "INFO";
            case LogLevel::Warning: return "WARN";
            case LogLevel::Error: return "ERROR";
            case LogLevel::Critical: return "FATAL";
        }
        return "UNKNOWN";
    }

    static ColorVariant levelColor(LogLevel level)
    {
        switch (level)
        {
            case LogLevel::Trace: return ColorVariant::Default;
            case LogLevel::Debug: return ColorVariant::Info;
            case LogLevel::Info: return ColorVariant::Success;
            case LogLevel::Warning: return ColorVariant::Warning;
            case LogLevel::Error:
            case LogLevel::Critical: return ColorVariant::Error;
        }
        return ColorVariant::Default;
    }

    static bool containsIgnoreAsciiCase(std::string_view haystack, std::string_view needle)
    {
        const auto fold = [](unsigned char ch) -> unsigned char {
            return ch >= 'A' && ch <= 'Z' ? static_cast<unsigned char>(ch-'A'+'a') : ch;
        };
        return std::search(haystack.begin(),haystack.end(),needle.begin(),needle.end(),
            [&](char left,char right) {
                return fold(static_cast<unsigned char>(left)) == fold(static_cast<unsigned char>(right));
            }) != haystack.end();
    }

    static Component create(Props props)
    {
        auto indices = std::make_shared<std::vector<std::size_t>>();
        if (props.buffer)
        {
            for (std::size_t index = 0; index < props.buffer->size(); ++index)
            {
                const auto& entry = props.buffer->at(index);
                if (static_cast<int>(entry.level) < static_cast<int>(props.min_level)) continue;
                if (!props.search.empty() && !containsIgnoreAsciiCase(entry.message,props.search) &&
                    !containsIgnoreAsciiCase(entry.timestamp,props.search) &&
                    !containsIgnoreAsciiCase(levelName(entry.level),props.search)) continue;
                // VirtualList indexes are int, so never produce an unrepresentable count.
                if (indices->size() == static_cast<std::size_t>(std::numeric_limits<int>::max())) break;
                indices->push_back(index);
            }
        }
        auto buffer = std::move(props.buffer);
        const bool show_timestamp = props.show_timestamp;
        const bool show_level = props.show_level;
        return Component({.type = Component::Type::VirtualList,
            .key = std::move(props.key),.layout = props.layout,
            .disabled = props.disabled,.scrollbar_visible = props.show_scrollbar,
            .virtual_item_count = static_cast<int>(indices->size()),
            .virtual_item_styled = [buffer,indices,show_timestamp,show_level](int index) -> StyledText {
                if (!buffer || index < 0 || static_cast<std::size_t>(index) >= indices->size()) return {};
                const auto& entry = buffer->at((*indices)[static_cast<std::size_t>(index)]);
                StyledText result;
                if (show_timestamp && !entry.timestamp.empty())
                    result.push_back({.text = entry.timestamp+"  "});
                if (show_level)
                    result.push_back({.text = levelName(entry.level)+"  ",
                        .color = levelColor(entry.level),.bold = entry.level >= LogLevel::Error});
                result.push_back({.text = entry.message});
                return result;
            },
            .virtual_selected_index = props.selected_index,
            .virtual_follow_tail = props.follow_tail,
            .virtual_on_select = std::move(props.on_select)});
    }
}; /* End of class LogView */

} /* End of namespace terion */

#endif /* TERION_COMPONENTS_LOG_VIEW_H */
