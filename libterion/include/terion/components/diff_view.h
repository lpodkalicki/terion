/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#ifndef TERION_COMPONENTS_DIFF_VIEW_H
#define TERION_COMPONENTS_DIFF_VIEW_H

#include <algorithm>
#include <limits>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include <terion/components/virtual_list.h>
#include <terion/components/text_view_detail.h>

namespace terion {

// Read-only unified diff viewer. Expects unified diff input (e.g. git diff).
class DiffView : public Component
{
public:
    struct Props
    {
        std::string diff = {};
        bool show_line_numbers = true;
        std::string key = {};
        LayoutStyle layout = {};
        bool disabled = false;
        bool show_scrollbar = true;
        bool wrap = false; // By default preserve the exact unified diff line layout.
        int wrap_width = 0;
    }; /* End of struct Props */

    explicit DiffView(Props props) : Component(create(std::move(props))) {}

    [[nodiscard]] static std::vector<StyledText> render(std::string_view diff,
                                                         bool show_line_numbers = true,int wrap_width = 0)
    {
        std::vector<StyledText> lines;
        std::size_t begin = 0;
        int old_line = 0;
        int new_line = 0;
        bool have_hunk = false;
        while (begin < diff.size())
        {
            const auto end = diff.find('\n',begin);
            auto line = diff.substr(begin,end == std::string_view::npos ? diff.size()-begin : end-begin);
            if (!line.empty() && line.back() == '\r') line.remove_suffix(1);
            ColorVariant color = ColorVariant::Default;
            bool bold = false;
            std::string prefix;
            if (startsWith(line,"diff --git ")) have_hunk = false;
            if (startsWith(line,"@@"))
            {
                int old_start = 0;
                int new_start = 0;
                have_hunk = parseHunk(line,old_start,new_start);
                if (have_hunk) { old_line = old_start; new_line = new_start; }
                color = ColorVariant::Info;
                bold = true;
            }
            else if (startsWith(line,"diff --git ") || startsWith(line,"index ") ||
                     startsWith(line,"--- ") || startsWith(line,"+++ ") ||
                     startsWith(line,"new file mode ") || startsWith(line,"deleted file mode "))
            {
                color = ColorVariant::Primary;
                bold = true;
            }
            else if (!line.empty() && line.front() == '+' && have_hunk)
            {
                color = ColorVariant::Success;
                if (show_line_numbers) prefix = numberColumn(0,new_line);
                ++new_line;
            }
            else if (!line.empty() && line.front() == '-' && have_hunk)
            {
                color = ColorVariant::Error;
                if (show_line_numbers) prefix = numberColumn(old_line,0);
                ++old_line;
            }
            else if (!line.empty() && line.front() == ' ' && have_hunk)
            {
                if (show_line_numbers) prefix = numberColumn(old_line,new_line);
                ++old_line;
                ++new_line;
            }
            else if (startsWith(line,"\\ No newline")) color = ColorVariant::Warning;
            StyledText row;
            if (!prefix.empty()) row.push_back({.text = std::move(prefix),.color = ColorVariant::Info});
            row.push_back({.text = std::string(line),.color = color,.bold = bold});
            lines.push_back(std::move(row));
            if (lines.size() >= static_cast<std::size_t>(std::numeric_limits<int>::max())) break;
            if (end == std::string_view::npos) break;
            begin = end+1;
        }
        return detail::wrapLines(lines,wrap_width);
    }

private:
    static bool startsWith(std::string_view text,std::string_view prefix)
    {
        return text.substr(0,prefix.size()) == prefix;
    }

    static bool parseNumber(std::string_view line,std::size_t& index,int& value)
    {
        if (index >= line.size() || line[index] < '0' || line[index] > '9') return false;
        value = 0;
        while (index < line.size() && line[index] >= '0' && line[index] <= '9')
        {
            const int digit = line[index++]-'0';
            if (value > (std::numeric_limits<int>::max()-digit)/10) return false;
            value = value*10+digit;
        }
        return true;
    }

    static bool parseHunk(std::string_view line,int& old_start,int& new_start)
    {
        auto index = line.find('-');
        if (index == std::string_view::npos) return false;
        ++index;
        if (!parseNumber(line,index,old_start)) return false;
        const auto plus = line.find('+',index);
        if (plus == std::string_view::npos) return false;
        index = plus+1;
        return parseNumber(line,index,new_start);
    }

    static std::string numberColumn(int old_number,int new_number)
    {
        const auto field = [](int number) {
            auto value = number > 0 ? std::to_string(number) : std::string{};
            if (value.size() < 5) value.insert(0,5-value.size(),' ');
            return value;
        };
        return field(old_number)+" "+field(new_number)+" │ ";
    }

    static Component create(Props props)
    {
        auto lines = std::make_shared<const std::vector<StyledText>>(
            render(props.diff,props.show_line_numbers,props.wrap ? (props.wrap_width > 0 ?
                props.wrap_width : std::max(0,props.layout.width.value_or(0)-
                (props.show_scrollbar ? 1 : 0))) : 0));
        return VirtualList({.item_count = static_cast<int>(lines->size()),
            .item_styled = [lines](int index) -> StyledText {
                return (*lines)[static_cast<std::size_t>(index)];
            },.key = std::move(props.key),.layout = props.layout,
            .disabled = props.disabled,.show_scrollbar = props.show_scrollbar});
    }
}; /* End of class DiffView */

} /* End of namespace terion */

#endif /* TERION_COMPONENTS_DIFF_VIEW_H */
