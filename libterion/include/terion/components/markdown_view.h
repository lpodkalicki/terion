/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#ifndef TERION_COMPONENTS_MARKDOWN_VIEW_H
#define TERION_COMPONENTS_MARKDOWN_VIEW_H

#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include <terion/components/virtual_list.h>
#include <terion/components/text_view_detail.h>
#include <functional>

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
        // 0-based source line corresponding to the selected rendered line.
        std::function<void(std::size_t)> on_source_line_click = {};
    }; /* End of struct Props */

    explicit MarkdownView(Props props) : Component(create(std::move(props))) {}

    struct Heading
    {
        std::size_t source_line = 0;
        int level = 1;
        std::string text;
    };

    // Returns an outline in source order (0-based source lines), ignoring fenced code.
    [[nodiscard]] static std::vector<Heading> outline(std::string_view markdown);

    [[nodiscard]] static std::vector<StyledText> render(std::string_view markdown,
                                                         int wrap_width = 0);

private:
    struct Document
    {
        std::vector<StyledText> lines;
        std::vector<std::size_t> origins;
    };

    static std::vector<std::string_view> sourceLines(std::string_view text);

    static std::string_view trim(std::string_view value);

    static std::pair<char,std::size_t> fenceMarker(std::string_view line);

    static int setextLevel(std::string_view line);

    static std::vector<std::string_view> tableCells(std::string_view line);

    static bool tableSeparator(std::string_view line,std::size_t columns);

    static std::vector<TableAlignment> tableAlignments(std::string_view separator);

    static std::string plainText(const StyledText& spans);

    static StyledText clipCell(const StyledText& text,int width);

    static void appendTableRow(StyledText& output,const std::vector<std::string_view>& cells,
                               const std::vector<int>& widths,const std::vector<TableAlignment>& alignments,
                               bool header);

    static StyledText renderNestedLine(std::string_view line);

    static Document renderDocument(std::string_view markdown,int wrap_width);

    static bool startsWith(std::string_view value,std::string_view prefix);

    static std::string_view trimLeft(std::string_view value);

    static StyledText inlineText(std::string_view text,ColorVariant base = ColorVariant::Default,
                                 bool base_bold = false);

    static StyledText highlightCode(std::string_view line,std::string_view language);

    static StyledText renderLine(std::string_view line);

    struct DynamicDocument
    {
        std::string markdown;
        Document rendered;
        int width = -1;
        bool wrap = true;
        int fixed_width = 0;
    };

    static Component create(Props props);
}; /* End of class MarkdownView */

} /* End of namespace terion */

#endif /* TERION_COMPONENTS_MARKDOWN_VIEW_H */
