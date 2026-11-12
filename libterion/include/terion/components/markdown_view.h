/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#ifndef TERION_COMPONENTS_MARKDOWN_VIEW_H
#define TERION_COMPONENTS_MARKDOWN_VIEW_H

#include <algorithm>
#include <limits>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include <terion/components/virtual_list.h>
#include <terion/components/text_view_detail.h>
#include <cctype>

namespace terion {

// A terminal-oriented Markdown renderer. Deliberately not a full CommonMark parser:
// it renders readable text without HTML, images or terminal escape processing.
class MarkdownView : public Component
{
public:
    struct Props
    {
        std::string markdown = {};
        std::string key = {};
        LayoutStyle layout = {};
        bool disabled = false;
        bool show_scrollbar = true;
        bool wrap = true;
        int wrap_width = 0; // 0: infer from layout.width; positive: explicit columns
    }; /* End of struct Props */

    explicit MarkdownView(Props props) : Component(create(std::move(props))) {}

    [[nodiscard]] static std::vector<StyledText> render(std::string_view markdown,
                                                         int wrap_width = 0)
    {
        return renderDocument(markdown,wrap_width).lines;
    }

private:
    struct Document
    {
        std::vector<StyledText> lines;
        std::vector<std::size_t> origins;
    };

    static std::vector<std::string_view> sourceLines(std::string_view text)
    {
        std::vector<std::string_view> result;
        std::size_t begin = 0;
        while (begin < text.size())
        {
            const auto end = text.find('\n',begin);
            auto line = text.substr(begin,end == std::string_view::npos ? text.size()-begin : end-begin);
            if (!line.empty() && line.back() == '\r') line.remove_suffix(1);
            result.push_back(line);
            if (end == std::string_view::npos) break;
            begin = end+1;
        }
        return result;
    }

    static std::string_view trim(std::string_view value)
    {
        const auto first = value.find_first_not_of(" \t");
        if (first == std::string_view::npos) return {};
        value.remove_prefix(first);
        const auto last = value.find_last_not_of(" \t");
        return value.substr(0,last+1);
    }

    static std::vector<std::string_view> tableCells(std::string_view line)
    {
        std::vector<std::string_view> result;
        line = trim(line);
        if (!line.empty() && line.front() == '|') line.remove_prefix(1);
        if (!line.empty() && line.back() == '|') line.remove_suffix(1);
        std::size_t begin = 0;
        for (std::size_t i = 0; i <= line.size(); ++i)
            if (i == line.size() || (line[i] == '|' && (i == 0 || line[i-1] != '\\')))
            {
                result.push_back(trim(line.substr(begin,i-begin)));
                begin = i+1;
            }
        return result;
    }

    static bool tableSeparator(std::string_view line,std::size_t columns)
    {
        const auto cells = tableCells(line);
        if (cells.size() != columns || columns == 0) return false;
        for (auto cell : cells)
        {
            if (!cell.empty() && cell.front() == ':') cell.remove_prefix(1);
            if (!cell.empty() && cell.back() == ':') cell.remove_suffix(1);
            if (cell.size() < 3 || cell.find_first_not_of('-') != std::string_view::npos) return false;
        }
        return true;
    }

    static std::string plainText(const StyledText& spans)
    {
        std::string value;
        for (const auto& span : spans) value += span.text;
        return value;
    }

    static StyledText clipCell(const StyledText& text,int width)
    {
        StyledText result;
        int used = 0;
        for (const auto& span : text)
        {
            std::string bytes;
            for (std::size_t i = 0; i < span.text.size();)
            {
                const auto glyph = decodeUtf8(span.text,i);
                if (!glyph.bytes) break;
                const int cells = codePointWidth(glyph.code_point);
                if (used+cells > width) break;
                bytes += span.text.substr(i,glyph.bytes);
                used += cells;
                i += glyph.bytes;
            }
            if (!bytes.empty()) result.push_back({.text = std::move(bytes),
                .color = span.color,.bold = span.bold});
            if (used >= width) break;
        }
        return result;
    }

    static void appendTableRow(StyledText& output,const std::vector<std::string_view>& cells,
                               const std::vector<int>& widths,bool header)
    {
        output.push_back({.text = "│ ",.color = ColorVariant::Info});
        for (std::size_t i = 0; i < widths.size(); ++i)
        {
            const auto cell = i < cells.size() ? cells[i] : std::string_view{};
            auto styled = clipCell(inlineText(cell,header ? ColorVariant::Primary : ColorVariant::Default,header),widths[i]);
            const auto text = plainText(styled);
            const int visible = detail::textCells(text);
            for (auto& span : styled) output.push_back(std::move(span));
            if (visible < widths[i])
                output.push_back({.text = std::string(static_cast<std::size_t>(widths[i]-visible),' ')});
            output.push_back({.text = " │",.color = ColorVariant::Info});
            if (i+1 < widths.size()) output.push_back({.text = " "});
        }
    }

    static StyledText renderNestedLine(std::string_view line)
    {
        std::size_t spaces = 0;
        while (spaces < line.size() && (line[spaces] == ' ' || line[spaces] == '\t'))
            spaces += 1;
        const auto value = line.substr(spaces);
        // Only list items have a meaningful hierarchy. Do not indent ordinary paragraphs.
        const bool bullet = startsWith(value,"- ") || startsWith(value,"* ") ||
            startsWith(value,"+ ");
        std::size_t digits = 0;
        while (digits < value.size() && value[digits] >= '0' && value[digits] <= '9') ++digits;
        const bool numbered = digits && digits+1 < value.size() && value[digits] == '.' && value[digits+1] == ' ';
        if (spaces == 0 || (!bullet && !numbered)) return renderLine(line);
        const auto depth = std::min<std::size_t>(spaces/2,8);
        auto result = renderLine(value);
        if (!result.empty()) result.front().text.insert(0,depth*2,' ');
        return result;
    }

    static Document renderDocument(std::string_view markdown,int wrap_width)
    {
        Document document;
        const auto source = sourceLines(markdown);
        const auto emit = [&](const StyledText& line,std::size_t origin,int hanging = -1) {
            const auto wrapped = wrap_width > 0 ?
                (hanging >= 0 ? detail::wrapStyled(line,wrap_width,hanging) :
                    detail::wrapLines({line},wrap_width)) : std::vector<StyledText>{line};
            for (const auto& row : wrapped)
            {
                document.lines.push_back(row);
                document.origins.push_back(origin);
            }
        };
        bool fenced = false;
        std::string fence_marker;
        std::string language;
        for (std::size_t i = 0; i < source.size(); ++i)
        {
            const auto line = source[i];
            const auto trimmed = trimLeft(line);
            if (startsWith(trimmed,"```") || startsWith(trimmed,"~~~"))
            {
                const auto marker = std::string(trimmed.substr(0,3));
                if (!fenced || marker == fence_marker)
                {
                    fenced = !fenced;
                    fence_marker = fenced ? marker : std::string{};
                    language = fenced ? std::string(trim(trimmed.substr(3))) : std::string{};
                    // Fence delimiters are Markdown syntax, not visible preview text.
                    continue;
                }
            }
            if (fenced)
            {
                // Syntax highlighting is preserved; code stays left aligned.
                emit(highlightCode(line,language),i,0);
                continue;
            }
            if (i+1 < source.size() && line.find('|') != std::string_view::npos)
            {
                const auto headers = tableCells(line);
                if (tableSeparator(source[i+1],headers.size()))
                {
                    std::size_t last = i+2;
                    while (last < source.size() && source[last].find('|') != std::string_view::npos &&
                        !trim(source[last]).empty()) ++last;
                    std::vector<int> widths(headers.size(),3);
                    const auto update = [&](const std::vector<std::string_view>& cells) {
                        for (std::size_t c = 0; c < widths.size() && c < cells.size(); ++c)
                            widths[c] = std::min(32,std::max(widths[c],detail::textCells(
                                plainText(inlineText(cells[c])))));
                    };
                    update(headers);
                    for (std::size_t j = i+2; j < last; ++j) update(tableCells(source[j]));
                    StyledText header;
                    appendTableRow(header,headers,widths,true);
                    emit(header,i);
                    std::string rule = "├";
                    for (std::size_t c = 0; c < widths.size(); ++c)
                    {
                        for (int k = 0; k < widths[c]+2; ++k) rule += "─";
                        rule += c+1 < widths.size() ? "┼" : "┤";
                    }
                    emit({{.text = std::move(rule),.color = ColorVariant::Info}},i+1);
                    for (std::size_t j = i+2; j < last; ++j)
                    {
                        StyledText row;
                        appendTableRow(row,tableCells(source[j]),widths,false);
                        emit(row,j);
                    }
                    i = last-1;
                    continue;
                }
            }
            emit(renderNestedLine(line),i);
            if (document.lines.size() >= static_cast<std::size_t>(std::numeric_limits<int>::max())) break;
        }
        return document;
    }

    static bool startsWith(std::string_view value,std::string_view prefix)
    {
        return value.substr(0,prefix.size()) == prefix;
    }

    static std::string_view trimLeft(std::string_view value)
    {
        const auto offset = value.find_first_not_of(" \t");
        return offset == std::string_view::npos ? std::string_view{} : value.substr(offset);
    }

    static StyledText inlineText(std::string_view text,ColorVariant base = ColorVariant::Default,
                                 bool base_bold = false)
    {
        StyledText spans;
        std::size_t cursor = 0;
        std::string plain;
        const auto flush = [&] {
            if (!plain.empty())
            {
                spans.push_back({.text = std::move(plain),.color = base,.bold = base_bold});
                plain.clear();
            }
        };
        while (cursor < text.size())
        {
            if (text[cursor] == '\\' && cursor+1 < text.size())
            {
                plain.push_back(text[cursor+1]);
                cursor += 2;
                continue;
            }
            if (text[cursor] == '`')
            {
                const auto close = text.find('`',cursor+1);
                if (close != std::string_view::npos)
                {
                    flush();
                    spans.push_back({.text = std::string(text.substr(cursor+1,close-cursor-1)),
                        .color = ColorVariant::Info});
                    cursor = close+1;
                    continue;
                }
            }
            if (cursor+1 < text.size() && (startsWith(text.substr(cursor),"**") ||
                startsWith(text.substr(cursor),"__")))
            {
                const auto delimiter = text.substr(cursor,2);
                const auto close = text.find(delimiter,cursor+2);
                if (close != std::string_view::npos && close > cursor+2)
                {
                    flush();
                    spans.push_back({.text = std::string(text.substr(cursor+2,close-cursor-2)),
                        .color = base,.bold = true});
                    cursor = close+2;
                    continue;
                }
            }
            if (startsWith(text.substr(cursor),"~~"))
            {
                const auto close = text.find("~~",cursor+2);
                if (close != std::string_view::npos && close > cursor+2)
                {
                    flush();
                    spans.push_back({.text = std::string(text.substr(cursor+2,close-cursor-2)),
                        .color = ColorVariant::Warning});
                    cursor = close+2;
                    continue;
                }
            }
            if (text[cursor] == '*' || text[cursor] == '_')
            {
                const auto close = text.find(text[cursor],cursor+1);
                if (close != std::string_view::npos && close > cursor+1)
                {
                    flush();
                    spans.push_back({.text = std::string(text.substr(cursor+1,close-cursor-1)),
                        .color = ColorVariant::Info,.bold = base_bold});
                    cursor = close+1;
                    continue;
                }
            }
            if (text[cursor] == '[')
            {
                const auto bracket = text.find("](",cursor+1);
                if (bracket != std::string_view::npos)
                {
                    const auto end = text.find(')',bracket+2);
                    if (end != std::string_view::npos)
                    {
                        flush();
                        spans.push_back({.text = std::string(text.substr(cursor+1,bracket-cursor-1)),
                            .color = ColorVariant::Primary});
                        cursor = end+1;
                        continue;
                    }
                }
            }
            plain.push_back(text[cursor++]);
        }
        flush();
        return spans;
    }

    static StyledText highlightCode(std::string_view line,std::string_view language)
    {
        StyledText result = {{.text = "  ",.color = ColorVariant::Info}};
        if (language != "cpp" && language != "c++" && language != "c" &&
            language != "python" && language != "py" && language != "bash" &&
            language != "sh" && language != "json" && language != "js" &&
            language != "javascript" && language != "ts" && language != "typescript")
        {
            detail::appendSpan(result,std::string(line),ColorVariant::Info,false);
            return result;
        }
        const auto word = [](const char c) {
            return std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '_';
        };
        const auto keyword = [language](std::string_view token) {
            constexpr std::string_view common = " if else for while return class struct const auto void int bool float double true false nullptr new delete def import from as in is None True False function let var export async await interface type public private protected include namespace using template typename ";
            const std::string value = " "+std::string(token)+" ";
            if (common.find(value) != std::string_view::npos) return true;
            return (language == "bash" || language == "sh") &&
                std::string_view(" echo fi then do done local export source ").find(value) != std::string_view::npos;
        };
        for (std::size_t i = 0; i < line.size();)
        {
            if ((line[i] == '#' && (language == "python" || language == "py" ||
                language == "bash" || language == "sh")) ||
                line.substr(i,2) == "//")
            {
                detail::appendSpan(result,std::string(line.substr(i)),ColorVariant::Success,false);
                break;
            }
            if (line[i] == '\"' || line[i] == '\'')
            {
                const auto quote = line[i];
                const auto start = i++;
                while (i < line.size())
                {
                    if (line[i] == '\\' && i+1 < line.size()) { i += 2; continue; }
                    if (line[i++] == quote) break;
                }
                detail::appendSpan(result,std::string(line.substr(start,i-start)),ColorVariant::Warning,false);
                continue;
            }
            if (word(line[i]))
            {
                const auto start = i++;
                while (i < line.size() && word(line[i])) ++i;
                const auto token = line.substr(start,i-start);
                const auto color = keyword(token) ? ColorVariant::Primary :
                    (std::isdigit(static_cast<unsigned char>(token.front())) != 0 ?
                        ColorVariant::Warning : ColorVariant::Info);
                detail::appendSpan(result,std::string(token),color,keyword(token));
                continue;
            }
            detail::appendSpan(result,std::string(1,line[i++]),ColorVariant::Info,false);
        }
        return result;
    }

    static StyledText renderLine(std::string_view line)
    {
        const auto value = trimLeft(line);
        if (value.empty()) return {};
        std::size_t hashes = 0;
        while (hashes < value.size() && hashes < 6 && value[hashes] == '#') ++hashes;
        if (hashes > 0 && hashes < value.size() && value[hashes] == ' ')
        {
            StyledText result = {{.text = std::string(hashes == 1 ? "▌ " : "  "),
                .color = ColorVariant::Primary,.bold = true}};
            auto content = inlineText(value.substr(hashes+1),ColorVariant::Primary,true);
            result.insert(result.end(),content.begin(),content.end());
            return result;
        }
        if (startsWith(value,"> "))
        {
            StyledText result = {{.text = "│ ",.color = ColorVariant::Info}};
            auto content = inlineText(value.substr(2));
            result.insert(result.end(),content.begin(),content.end());
            return result;
        }
        if (value == "---" || value == "***" || value == "___")
            return {{.text = "────────────────────────",.color = ColorVariant::Info}};
        if (startsWith(value,"- [ ] ") || startsWith(value,"* [ ] "))
        {
            StyledText result = {{.text = "  □ ",.color = ColorVariant::Info}};
            auto content = inlineText(value.substr(6));
            result.insert(result.end(),content.begin(),content.end());
            return result;
        }
        if (startsWith(value,"- [x] ") || startsWith(value,"- [X] ") ||
            startsWith(value,"* [x] ") || startsWith(value,"* [X] "))
        {
            StyledText result = {{.text = "  ☑ ",.color = ColorVariant::Success}};
            auto content = inlineText(value.substr(6));
            result.insert(result.end(),content.begin(),content.end());
            return result;
        }
        if (startsWith(value,"- ") || startsWith(value,"* ") || startsWith(value,"+ "))
        {
            StyledText result = {{.text = "  • ",.color = ColorVariant::Primary}};
            auto content = inlineText(value.substr(2));
            result.insert(result.end(),content.begin(),content.end());
            return result;
        }
        std::size_t digits = 0;
        while (digits < value.size() && value[digits] >= '0' && value[digits] <= '9') ++digits;
        if (digits > 0 && digits+1 < value.size() && value[digits] == '.' && value[digits+1] == ' ')
        {
            StyledText result = {{.text = "  "+std::string(value.substr(0,digits+1))+" ",
                .color = ColorVariant::Primary}};
            auto content = inlineText(value.substr(digits+2));
            result.insert(result.end(),content.begin(),content.end());
            return result;
        }
        return inlineText(line);
    }

    struct DynamicDocument
    {
        std::string markdown;
        Document rendered;
        int width = -1;
        bool wrap = true;
        int fixed_width = 0;
    };

    static Component create(Props props)
    {
        auto state = std::make_shared<DynamicDocument>();
        state->markdown = std::move(props.markdown);
        state->wrap = props.wrap;
        state->fixed_width = props.wrap_width;
        const int initial_width = props.wrap ? (props.wrap_width > 0 ? props.wrap_width :
            std::max(0,props.layout.width.value_or(0)-(props.show_scrollbar ? 1 : 0))) : 0;
        state->rendered = renderDocument(state->markdown,initial_width);
        state->width = initial_width;
        return VirtualList({.item_count = static_cast<int>(state->rendered.lines.size()),
            .item_styled = [state](int index) -> StyledText {
                return state->rendered.lines[static_cast<std::size_t>(index)];
            },.on_width_change = [state](int width,int offset) -> std::pair<int,int> {
                const int target = state->wrap ? (state->fixed_width > 0 ? state->fixed_width : width) : 0;
                if (target != state->width)
                {
                    std::size_t origin = 0;
                    if (offset >= 0 && static_cast<std::size_t>(offset) < state->rendered.origins.size())
                        origin = state->rendered.origins[static_cast<std::size_t>(offset)];
                    state->rendered = renderDocument(state->markdown,target);
                    state->width = target;
                    const auto it = std::lower_bound(state->rendered.origins.begin(),
                        state->rendered.origins.end(),origin);
                    offset = static_cast<int>(it-state->rendered.origins.begin());
                }
                return {static_cast<int>(state->rendered.lines.size()),offset};
            },.key = std::move(props.key),.layout = props.layout,
            .disabled = props.disabled,.show_scrollbar = props.show_scrollbar});
    }
}; /* End of class MarkdownView */

} /* End of namespace terion */

#endif /* TERION_COMPONENTS_MARKDOWN_VIEW_H */
