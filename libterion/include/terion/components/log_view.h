/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#ifndef TERION_COMPONENTS_LOG_VIEW_H
#define TERION_COMPONENTS_LOG_VIEW_H

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <limits>
#include <memory>
#include <optional>
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
    // Stable within a buffer, including after its oldest entries are evicted.
    std::uint64_t sequence = 0;
}; /* End of struct LogEntry */

// UI-thread-owned bounded buffer. Append and rendering run on the UI thread.
class LogBuffer
{
public:
    explicit LogBuffer(std::size_t capacity = 10000) : capacity_(capacity) {}

    void append(LogEntry entry)
    {
        if (capacity_ == 0) return;
        entry.sequence = ++last_sequence_;
        if (entries_.size() == capacity_) entries_.pop_front();
        entries_.push_back(std::move(entry));
        ++revision_;
    }

    void clear() noexcept { entries_.clear(); ++revision_; }
    [[nodiscard]] std::size_t size() const noexcept { return entries_.size(); }
    [[nodiscard]] std::size_t capacity() const noexcept { return capacity_; }
    [[nodiscard]] std::uint64_t revision() const noexcept { return revision_; }
    [[nodiscard]] const LogEntry& at(std::size_t index) const { return entries_.at(index); }
    [[nodiscard]] std::shared_ptr<const LogBuffer> snapshot() const
    {
        return std::make_shared<const LogBuffer>(*this);
    }

private:
    std::size_t capacity_;
    std::deque<LogEntry> entries_;
    std::uint64_t last_sequence_ = 0;
    std::uint64_t revision_ = 0;
}; /* End of class LogBuffer */

struct LogFilter
{
    LogLevel min_level = LogLevel::Trace;
    std::string search = {};
    // Empty means all severities at or above min_level.
    std::vector<LogLevel> levels = {};
    bool operator==(const LogFilter&) const = default;
};

namespace log_detail {

inline unsigned char foldAscii(unsigned char ch)
{
    return ch >= 'A' && ch <= 'Z' ? static_cast<unsigned char>(ch-'A'+'a') : ch;
}

inline std::size_t findAsciiCaseInsensitive(std::string_view text,std::string_view needle,
    std::size_t from = 0)
{
    if (needle.empty()) return from <= text.size() ? from : std::string_view::npos;
    if (from > text.size()) return std::string_view::npos;
    const auto begin = std::search(text.begin()+static_cast<std::ptrdiff_t>(from),text.end(),
        needle.begin(),needle.end(),[](char a,char b) {
            return foldAscii(static_cast<unsigned char>(a)) == foldAscii(static_cast<unsigned char>(b));
        });
    return begin == text.end() ? std::string_view::npos :
        static_cast<std::size_t>(begin-text.begin());
}

inline std::string levelName(LogLevel level)
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

inline ColorVariant levelColor(LogLevel level)
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

inline std::vector<std::size_t> filterIndices(const LogBuffer& buffer,const LogFilter& filter)
{
    std::vector<std::size_t> result;
    for (std::size_t i = 0; i < buffer.size(); ++i)
    {
        const auto& entry = buffer.at(i);
        if (static_cast<int>(entry.level) < static_cast<int>(filter.min_level)) continue;
        if (!filter.levels.empty() && std::find(filter.levels.begin(),filter.levels.end(),entry.level)
            == filter.levels.end()) continue;
        if (!filter.search.empty() &&
            findAsciiCaseInsensitive(entry.message,filter.search) == std::string_view::npos &&
            findAsciiCaseInsensitive(entry.timestamp,filter.search) == std::string_view::npos &&
            findAsciiCaseInsensitive(levelName(entry.level),filter.search) == std::string_view::npos) continue;
        if (result.size() == static_cast<std::size_t>(std::numeric_limits<int>::max())) break;
        result.push_back(i);
    }
    return result;
}

inline void appendHighlighted(StyledText& spans,std::string_view text,ColorVariant color,
    bool bold,std::string_view search)
{
    if (search.empty())
    {
        spans.push_back({.text=std::string(text),.color=color,.bold=bold});
        return;
    }
    std::size_t begin = 0;
    while (begin < text.size())
    {
        const auto match = findAsciiCaseInsensitive(text,search,begin);
        if (match == std::string_view::npos)
        {
            spans.push_back({.text=std::string(text.substr(begin)),.color=color,.bold=bold});
            break;
        }
        if (match > begin)
            spans.push_back({.text=std::string(text.substr(begin,match-begin)),.color=color,.bold=bold});
        spans.push_back({.text=std::string(text.substr(match,search.size())),.color=color,
            .bold=true,.underline=true,.background=ColorVariant::Warning});
        begin=match+search.size();
    }
}

inline std::string escapedTsv(std::string_view value)
{
    std::string result;
    result.reserve(value.size());
    for (char ch : value) result += ch == '\n' || ch == '\r' || ch == '\t' ? ' ' : ch;
    return result;
}

} /* End of namespace log_detail */

// Optional reusable filtered-index cache, invalidated by buffer revision, identity or query.
// Owned by the UI thread; does not access the buffer on a worker thread.
class LogFilterCache
{
public:
    [[nodiscard]] std::shared_ptr<const std::vector<std::size_t>> resolve(
        const std::shared_ptr<const LogBuffer>& buffer,const LogFilter& filter)
    {
        if (!buffer) return std::make_shared<const std::vector<std::size_t>>();
        if (auto current = buffer_.lock(); current == buffer &&
            revision_ == buffer->revision() && filter_ == filter && indices_) return indices_;
        buffer_ = buffer;
        revision_ = buffer->revision();
        filter_ = filter;
        indices_ = std::make_shared<const std::vector<std::size_t>>(
            log_detail::filterIndices(*buffer,filter));
        return indices_;
    }

    void invalidate() { buffer_.reset(); indices_.reset(); }

private:
    std::weak_ptr<const LogBuffer> buffer_;
    std::uint64_t revision_ = 0;
    LogFilter filter_ = {};
    std::shared_ptr<const std::vector<std::size_t>> indices_ = {};
};

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
        std::string id = {};
        LayoutStyle layout = {};
        bool disabled = false;
        bool show_scrollbar = true;
        std::vector<LogLevel> levels = {};
        bool highlight_matches = true;
        std::shared_ptr<LogFilterCache> filter_cache = {};
        std::optional<std::uint64_t> selected_sequence = std::nullopt;
        std::function<void(std::uint64_t)> on_select_sequence = {};
        std::function<void(std::uint64_t)> on_activate_sequence = {};
    }; /* End of struct Props */

    explicit LogView(Props props) : Component(create(std::move(props))) {}

    [[nodiscard]] static std::vector<std::size_t> filtered(const LogBuffer& buffer,
        const LogFilter& filter = {})
    {
        return log_detail::filterIndices(buffer,filter);
    }

    [[nodiscard]] static std::string exportTsv(const LogBuffer& buffer,const LogFilter& filter = {})
    {
        std::string output = "timestamp\tlevel\tmessage\n";
        for (const auto i : filtered(buffer,filter))
        {
            const auto& entry = buffer.at(i);
            output += log_detail::escapedTsv(entry.timestamp) + "\t" +
                log_detail::levelName(entry.level) + "\t" +
                log_detail::escapedTsv(entry.message) + "\n";
        }
        return output;
    }

private:
    static Component create(Props props)
    {
        auto buffer = std::move(props.buffer);
        const LogFilter filter{props.min_level,props.search,std::move(props.levels)};
        std::shared_ptr<const std::vector<std::size_t>> indices = props.filter_cache ?
            props.filter_cache->resolve(buffer,filter) :
            std::make_shared<const std::vector<std::size_t>>(buffer ?
                filtered(*buffer,filter) : std::vector<std::size_t>{});
        int selected = props.selected_index;
        if (props.selected_sequence && buffer)
        {
            selected = -1;
            for (std::size_t row = 0; row < indices->size(); ++row)
                if (buffer->at((*indices)[row]).sequence == *props.selected_sequence)
                {
                    selected = static_cast<int>(row);
                    break;
                }
        }
        const bool show_timestamp = props.show_timestamp;
        const bool show_level = props.show_level;
        const bool highlight = props.highlight_matches;
        const auto on_select = std::move(props.on_select);
        const auto on_select_sequence = std::move(props.on_select_sequence);
        const auto on_activate_sequence = std::move(props.on_activate_sequence);
        return VirtualList({.item_count = static_cast<int>(indices->size()),
            .item_styled = [buffer,indices,show_timestamp,show_level,highlight,search=filter.search](int index) -> StyledText {
                if (!buffer || index < 0 || static_cast<std::size_t>(index) >= indices->size()) return {};
                const auto& entry = buffer->at((*indices)[static_cast<std::size_t>(index)]);
                StyledText result;
                const auto append = [&](std::string_view text,ColorVariant color,bool bold) {
                    log_detail::appendHighlighted(result,text,color,bold,highlight ? search : "");
                };
                if (show_timestamp && !entry.timestamp.empty()) append(entry.timestamp+"  ",ColorVariant::Default,false);
                if (show_level) append(log_detail::levelName(entry.level)+"  ",
                    log_detail::levelColor(entry.level),entry.level >= LogLevel::Error);
                append(entry.message,ColorVariant::Default,false);
                return result;
            },
            .selected_index = selected,
            .follow_tail = props.follow_tail,
            .on_selection_change = [buffer,indices,on_select,on_select_sequence](int index) {
                if (on_select) on_select(index);
                if (on_select_sequence && buffer && index >= 0 &&
                    static_cast<std::size_t>(index) < indices->size())
                    on_select_sequence(buffer->at((*indices)[static_cast<std::size_t>(index)]).sequence);
            },
            .on_activate = [buffer,indices,on_activate_sequence](int index) {
                if (on_activate_sequence && buffer && index >= 0 &&
                    static_cast<std::size_t>(index) < indices->size())
                    on_activate_sequence(buffer->at((*indices)[static_cast<std::size_t>(index)]).sequence);
            },
            .key = std::move(props.key),.id = std::move(props.id),.layout = props.layout,.disabled = props.disabled,
            .show_scrollbar = props.show_scrollbar});
    }
}; /* End of class LogView */

} /* End of namespace terion */

#endif /* TERION_COMPONENTS_LOG_VIEW_H */
