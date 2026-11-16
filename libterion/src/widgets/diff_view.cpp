/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <terion/components/diff_view.h>
#include <algorithm>
#include <functional>
#include <limits>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include <terion/components/virtual_list.h>
#include <terion/components/text_view_detail.h>

namespace terion {

std::vector<StyledText> DiffView::render(
        std::string_view diff,DiffViewMode mode,
        bool show_line_numbers,int width,bool highlight_intraline)
{
        return renderDocument(diff,mode,show_line_numbers,width,highlight_intraline).lines;
    }

std::vector<DiffView::Hunk> DiffView::hunks(std::string_view diff)
{
        std::vector<Hunk> result;
        for (const auto& line : parse(diff))
            if (line.kind == Kind::Hunk)
                result.push_back({line.source_line,line.old_number,line.new_number,line.text});
        return result;
    }

std::string DiffView::hunkText(std::string_view diff,std::size_t hunk_index)
{
        const auto spans = hunks(diff);
        if (hunk_index >= spans.size()) return {};
        const auto lines = sourceLines(diff);
        const std::size_t begin_line = spans[hunk_index].source_line;
        std::size_t end_line = lines.size();
        if (hunk_index+1 < spans.size()) end_line = spans[hunk_index+1].source_line;
        for (std::size_t i = begin_line+1; i < end_line; ++i)
            if (startsWith(lines[i],"diff --git ")) { end_line = i; break; }
        const auto offsetOfLine = [diff](std::size_t line_index) {
            std::size_t offset = 0;
            for (std::size_t line = 0; line < line_index && offset < diff.size(); ++line)
            {
                const auto newline = diff.find('\n',offset);
                if (newline == std::string_view::npos) return diff.size();
                offset = newline+1;
            }
            return offset;
        };
        const auto begin = offsetOfLine(begin_line);
        const auto end = offsetOfLine(end_line);
        return std::string(diff.substr(begin,end-begin));
    }

bool DiffView::startsWith(std::string_view text,std::string_view prefix)
{
        return text.substr(0,prefix.size()) == prefix;
    }

std::vector<std::string> DiffView::sourceLines(std::string_view diff)
{
        std::vector<std::string> lines;
        std::size_t begin = 0;
        while (begin < diff.size())
        {
            const auto end = diff.find('\n',begin);
            auto line = diff.substr(begin,end == std::string_view::npos ? diff.size()-begin : end-begin);
            if (!line.empty() && line.back() == '\r') line.remove_suffix(1);
            lines.emplace_back(line);
            if (end == std::string_view::npos) break;
            begin = end+1;
        }
        return lines;
    }

bool DiffView::parseNumber(std::string_view line,std::size_t& index,int& value)
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

bool DiffView::parseHunk(std::string_view line,int& old_start,int& new_start)
{
        if (!startsWith(line,"@@ ")) return false;
        auto index = line.find('-');
        if (index == std::string_view::npos) return false;
        ++index;
        if (!parseNumber(line,index,old_start)) return false;
        const auto plus = line.find('+',index);
        if (plus == std::string_view::npos) return false;
        index = plus+1;
        return parseNumber(line,index,new_start);
    }

std::string DiffView::numberColumn(int old_number,int new_number)
{
        const auto field = [](int number) {
            auto value = number > 0 ? std::to_string(number) : std::string{};
            if (value.size() < 5) value.insert(0,5-value.size(),' ');
            return value;
        };
        return field(old_number)+" "+field(new_number)+" │ ";
    }

std::string DiffView::sideNumber(int number)
{
        auto text = number > 0 ? std::to_string(number) : std::string{};
        if (text.size() < 5) text.insert(0,5-text.size(),' ');
        return text+" ";
    }

std::vector<DiffView::SourceRow> DiffView::parse(std::string_view diff)
{
        std::vector<SourceRow> result;
        int old_line = 0;
        int new_line = 0;
        bool in_hunk = false;
        const auto lines = sourceLines(diff);
        for (std::size_t i = 0; i < lines.size(); ++i)
        {
            SourceRow row;
            row.text = lines[i];
            row.source_line = i;
            if (startsWith(row.text,"diff --git ")) in_hunk = false;
            if (startsWith(row.text,"@@ "))
            {
                in_hunk = parseHunk(row.text,old_line,new_line);
                if (in_hunk)
                {
                    row.kind = Kind::Hunk;
                    row.old_number = old_line;
                    row.new_number = new_line;
                }
            }
            else if (startsWith(row.text,"diff --git ") || startsWith(row.text,"index ") ||
                     startsWith(row.text,"--- ") || startsWith(row.text,"+++ ") ||
                     startsWith(row.text,"new file mode ") || startsWith(row.text,"deleted file mode ") ||
                     startsWith(row.text,"rename from ") || startsWith(row.text,"rename to "))
                row.kind = Kind::Header;
            else if (in_hunk && !row.text.empty() && row.text[0] == '-')
            {
                row.kind = Kind::Removed;
                row.old_number = old_line++;
            }
            else if (in_hunk && !row.text.empty() && row.text[0] == '+')
            {
                row.kind = Kind::Added;
                row.new_number = new_line++;
            }
            else if (in_hunk && !row.text.empty() && row.text[0] == ' ')
            {
                row.kind = Kind::Context;
                row.old_number = old_line++;
                row.new_number = new_line++;
            }
            else if (startsWith(row.text,"\\ No newline")) row.kind = Kind::Warning;
            result.push_back(std::move(row));
        }
        return result;
    }

ColorVariant DiffView::colorFor(Kind kind)
{
        switch (kind)
        {
            case Kind::Header: return ColorVariant::Primary;
            case Kind::Hunk: return ColorVariant::Info;
            case Kind::Removed: return ColorVariant::Error;
            case Kind::Added: return ColorVariant::Success;
            case Kind::Warning: return ColorVariant::Warning;
            default: return ColorVariant::Default;
        }
    }

StyledText DiffView::changedText(const SourceRow& row,const SourceRow* counterpart,
                                  bool highlight,bool include_marker)
{
        const ColorVariant color = colorFor(row.kind);
        if (!highlight || counterpart == nullptr || row.text.empty() || counterpart->text.empty())
            return {{.text = include_marker ? row.text : row.text.substr(1),.color = color}};
        const auto glyphs = [](std::string_view text) {
            std::vector<std::string> result;
            for (std::size_t pos = 0; pos < text.size();)
            {
                const auto glyph = nextGrapheme(text,pos);
                if (!glyph.bytes) break;
                result.emplace_back(text.substr(pos,glyph.bytes));
                pos += glyph.bytes;
            }
            return result;
        };
        const auto lhs = glyphs(std::string_view(row.text).substr(1));
        const auto rhs = glyphs(std::string_view(counterpart->text).substr(1));
        std::size_t prefix = 0;
        while (prefix < lhs.size() && prefix < rhs.size() && lhs[prefix] == rhs[prefix]) ++prefix;
        std::size_t suffix = 0;
        while (suffix < lhs.size()-prefix && suffix < rhs.size()-prefix &&
            lhs[lhs.size()-suffix-1] == rhs[rhs.size()-suffix-1]) ++suffix;
        StyledText spans;
        if (include_marker) spans.push_back({.text = row.text.substr(0,1),.color = color});
        const auto append = [&](std::size_t first,std::size_t last,bool changed) {
            std::string text;
            for (std::size_t i = first; i < last; ++i) text += lhs[i];
            if (!text.empty()) spans.push_back({.text = std::move(text),.color = color,
                .bold = changed,.underline = changed});
        };
        append(0,prefix,false);
        append(prefix,lhs.size()-suffix,true);
        append(lhs.size()-suffix,lhs.size(),false);
        return spans;
    }

StyledText DiffView::unifiedLine(const SourceRow& row,const SourceRow* counterpart,
                                  bool numbers,bool highlight)
{
        StyledText result;
        if (numbers && (row.kind == Kind::Removed || row.kind == Kind::Added || row.kind == Kind::Context))
            result.push_back({.text = numberColumn(row.old_number,row.new_number),.color = ColorVariant::Info});
        if (row.kind == Kind::Removed || row.kind == Kind::Added)
        {
            auto content = changedText(row,counterpart,highlight,true);
            result.insert(result.end(),std::make_move_iterator(content.begin()),
                          std::make_move_iterator(content.end()));
        }
        else
        {
            StyledSpan span;
            span.text = row.text;
            span.color = colorFor(row.kind);
            span.bold = row.kind == Kind::Header || row.kind == Kind::Hunk;
            result.push_back(std::move(span));
        }
        return result;
    }

StyledText DiffView::clipPad(const StyledText& source,int width)
{
        StyledText result;
        int used = 0;
        bool full = false;
        for (const auto& span : source)
        {
            if (full) break;
            StyledSpan clipped = span;
            clipped.text.clear();
            for (std::size_t pos = 0; pos < span.text.size();)
            {
                const auto glyph = nextGrapheme(span.text,pos);
                if (!glyph.bytes) break;
                if (used+glyph.columns > width) { full = true; break; }
                clipped.text += span.text.substr(pos,glyph.bytes);
                pos += glyph.bytes;
                used += glyph.columns;
            }
            if (!clipped.text.empty()) result.push_back(std::move(clipped));
            if (used >= width) break;
        }
        if (used < width) result.push_back({.text = std::string(static_cast<std::size_t>(width-used),' ')});
        return result;
    }

StyledText DiffView::sideLine(const SourceRow* left,const SourceRow* right,int width,bool numbers,
                               bool highlight)
{
        const int left_width = std::max(1,(width-3)/2);
        const int right_width = std::max(1,width-3-left_width);
        const auto side = [&](const SourceRow* row,const SourceRow* counterpart,int pane_width,bool old) {
            StyledText result;
            if (numbers) result.push_back({.text = sideNumber(row ? (old ? row->old_number : row->new_number) : 0),
                .color = ColorVariant::Info});
            if (row)
            {
                auto content = changedText(*row,counterpart,highlight,false);
                result.insert(result.end(),std::make_move_iterator(content.begin()),std::make_move_iterator(content.end()));
            }
            return clipPad(result,pane_width);
        };
        auto result = side(left,right,left_width,true);
        result.push_back({.text = " │ ",.color = ColorVariant::Info});
        auto rhs = side(right,left,right_width,false);
        result.insert(result.end(),std::make_move_iterator(rhs.begin()),std::make_move_iterator(rhs.end()));
        return result;
    }

DiffView::Document DiffView::renderDocument(std::string_view diff,DiffViewMode mode,bool numbers,int width,bool highlight)
{
        Document doc;
        const auto rows = parse(diff);
        const bool side_by_side = mode == DiffViewMode::SideBySide && width >= 18;
        const auto emit = [&](const StyledText& text,std::size_t origin) {
            auto wrapped = !side_by_side && width > 0 ? detail::wrapStyled(text,width) :
                std::vector<StyledText>{text};
            for (auto& line : wrapped)
            {
                doc.lines.push_back(std::move(line));
                doc.origins.push_back(origin);
            }
        };
        for (std::size_t i = 0; i < rows.size();)
        {
            const auto& row = rows[i];
            if (row.kind == Kind::Removed)
            {
                std::size_t end_removed = i;
                while (end_removed < rows.size() && rows[end_removed].kind == Kind::Removed) ++end_removed;
                std::size_t end_added = end_removed;
                while (end_added < rows.size() && rows[end_added].kind == Kind::Added) ++end_added;
                if (side_by_side)
                {
                    const auto count = std::max(end_removed-i,end_added-end_removed);
                    for (std::size_t k = 0; k < count; ++k)
                    {
                        const auto* left = i+k < end_removed ? &rows[i+k] : nullptr;
                        const auto* right = end_removed+k < end_added ? &rows[end_removed+k] : nullptr;
                        emit(sideLine(left,right,width,numbers,highlight),
                            left ? left->source_line : right->source_line);
                    }
                }
                else
                {
                    for (std::size_t k = i; k < end_added; ++k)
                    {
                        const auto& current = rows[k];
                        const SourceRow* partner = nullptr;
                        if (current.kind == Kind::Removed && end_removed+(k-i) < end_added)
                            partner = &rows[end_removed+(k-i)];
                        else if (current.kind == Kind::Added && i+(k-end_removed) < end_removed)
                            partner = &rows[i+(k-end_removed)];
                        emit(unifiedLine(current,partner,numbers,highlight),current.source_line);
                    }
                }
                i = end_added;
                continue;
            }
            if (side_by_side && row.kind == Kind::Added)
                emit(sideLine(nullptr,&row,width,numbers,false),row.source_line);
            else if (side_by_side && row.kind == Kind::Context)
                emit(sideLine(&row,&row,width,numbers,false),row.source_line);
            else if (side_by_side)
                emit(clipPad(unifiedLine(row,nullptr,false,false),width),row.source_line);
            else emit(unifiedLine(row,nullptr,numbers,false),row.source_line);
            ++i;
            if (doc.lines.size() >= static_cast<std::size_t>(std::numeric_limits<int>::max())) break;
        }
        return doc;
    }

Component DiffView::create(Props props)
{
        auto state = std::make_shared<DynamicDocument>();
        state->diff = std::move(props.diff);
        state->mode = props.mode;
        state->show_line_numbers = props.show_line_numbers;
        state->wrap = props.wrap;
        state->highlight = props.highlight_intraline;
        state->fixed_width = props.wrap_width;
        const auto locations = hunks(state->diff);
        if (props.active_hunk >= 0 && static_cast<std::size_t>(props.active_hunk) < locations.size())
            state->selected_origin = locations[static_cast<std::size_t>(props.active_hunk)].source_line;
        const int available = std::max(0,props.layout.width.value_or(0)-(props.show_scrollbar ? 1 : 0));
        const int requested = props.wrap_width > 0 ? props.wrap_width : available;
        const int initial_width = props.mode == DiffViewMode::SideBySide || props.wrap ? requested : 0;
        state->rendered = renderDocument(state->diff,state->mode,state->show_line_numbers,
            initial_width,state->highlight);
        state->width = initial_width;
        const auto selected_it = std::lower_bound(state->rendered.origins.begin(),
            state->rendered.origins.end(),state->selected_origin);
        const int selected_index = state->selected_origin == std::numeric_limits<std::size_t>::max() ||
            selected_it == state->rendered.origins.end() || *selected_it != state->selected_origin ?
            -1 : static_cast<int>(selected_it-state->rendered.origins.begin());
        return VirtualList({.item_count = static_cast<int>(state->rendered.lines.size()),
            .item_styled = [state](int index) -> StyledText {
                return state->rendered.lines[static_cast<std::size_t>(index)];
            },.on_width_change = [state](int columns,int offset) -> std::pair<int,int> {
                const int available_width = state->fixed_width > 0 ? state->fixed_width : columns;
                const int target = state->mode == DiffViewMode::SideBySide || state->wrap ? available_width : 0;
                if (target != state->width)
                {
                    std::size_t origin = 0;
                    if (offset >= 0 && static_cast<std::size_t>(offset) < state->rendered.origins.size())
                        origin = state->rendered.origins[static_cast<std::size_t>(offset)];
                    state->rendered = renderDocument(state->diff,state->mode,state->show_line_numbers,
                        target,state->highlight);
                    state->width = target;
                    const auto it = std::lower_bound(state->rendered.origins.begin(),
                        state->rendered.origins.end(),origin);
                    offset = static_cast<int>(it-state->rendered.origins.begin());
                }
                return {static_cast<int>(state->rendered.lines.size()),offset};
            },.selected_index = selected_index,
            .item_is_selected = [state](int index) {
                return index >= 0 && static_cast<std::size_t>(index) < state->rendered.origins.size() &&
                    state->rendered.origins[static_cast<std::size_t>(index)] == state->selected_origin;
            },.on_click_position = [state,callback = std::move(props.on_source_line_click)](int,int index) {
                if (callback && index >= 0 && static_cast<std::size_t>(index) < state->rendered.origins.size())
                    callback(state->rendered.origins[static_cast<std::size_t>(index)]);
            },.key = std::move(props.key),.layout = props.layout,
            .disabled = props.disabled,.show_scrollbar = props.show_scrollbar});
    }

} /* End of namespace terion */
