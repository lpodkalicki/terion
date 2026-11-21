/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#ifndef TERION_COMPONENTS_DIFF_VIEW_H
#define TERION_COMPONENTS_DIFF_VIEW_H

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include <terion/components/virtual_list.h>
#include <terion/components/text_view_detail.h>

namespace terion {

enum class DiffViewMode { Unified, SideBySide };

// A unified-diff viewer. The source is never changed by highlighting or navigation.
class DiffView : public Component
{
public:
    struct Hunk
    {
        std::size_t source_line = 0; // zero-based line in unified diff text
        int old_start = 0;
        int new_start = 0;
        std::string header;
    };

    struct Props
    {
        std::string diff = {};
        bool show_line_numbers = true;
        std::string key = {};
        std::string id = {};
        LayoutStyle layout = {};
        bool disabled = false;
        bool show_scrollbar = true;
        bool wrap = false;
        int wrap_width = 0;
        DiffViewMode mode = DiffViewMode::Unified;
        bool highlight_intraline = false;
        int active_hunk = -1; // zero-based index into hunks(); -1: none
        std::function<void(std::size_t)> on_source_line_click = {};
    };

    explicit DiffView(Props props) : Component(create(std::move(props))) {}

    [[nodiscard]] static std::vector<StyledText> render(
        std::string_view diff,DiffViewMode mode = DiffViewMode::Unified,
        bool show_line_numbers = true,int width = 0,bool highlight_intraline = false);

    [[nodiscard]] static std::vector<Hunk> hunks(std::string_view diff);

    // Return the original bytes of a hunk (including original newline encoding),
    // stopping at the next hunk or file boundary. Useful for Clipboard::set().
    [[nodiscard]] static std::string hunkText(std::string_view diff,std::size_t hunk_index);

private:
    enum class Kind { Header, Hunk, Context, Removed, Added, Warning, Other };

    struct SourceRow
    {
        std::string text;
        Kind kind = Kind::Other;
        int old_number = 0;
        int new_number = 0;
        std::size_t source_line = 0;
    };

    struct Document
    {
        std::vector<StyledText> lines;
        std::vector<std::size_t> origins;
    };

    static bool startsWith(std::string_view text,std::string_view prefix);

    static std::vector<std::string> sourceLines(std::string_view diff);

    static bool parseNumber(std::string_view line,std::size_t& index,int& value);

    static bool parseHunk(std::string_view line,int& old_start,int& new_start);

    static std::string numberColumn(int old_number,int new_number);

    static std::string sideNumber(int number);

    static std::vector<SourceRow> parse(std::string_view diff);

    static ColorVariant colorFor(Kind kind);

    // Intra-line comparison operates on grapheme clusters, never byte offsets.
    // Common prefix and suffix are left untouched; only changed graphemes are emphasized.
    static StyledText changedText(const SourceRow& row,const SourceRow* counterpart,
                                  bool highlight,bool include_marker);

    static StyledText unifiedLine(const SourceRow& row,const SourceRow* counterpart,
                                  bool numbers,bool highlight);

    static StyledText clipPad(const StyledText& source,int width);

    static StyledText sideLine(const SourceRow* left,const SourceRow* right,int width,bool numbers,
                               bool highlight);

    static Document renderDocument(std::string_view diff,DiffViewMode mode,bool numbers,int width,bool highlight);

    struct DynamicDocument
    {
        std::string diff;
        Document rendered;
        DiffViewMode mode = DiffViewMode::Unified;
        bool show_line_numbers = true;
        bool wrap = false;
        bool highlight = false;
        int fixed_width = 0;
        int width = -1;
        std::size_t selected_origin = std::numeric_limits<std::size_t>::max();
    };

    static Component create(Props props);
}; /* End of class DiffView */

} /* End of namespace terion */

#endif /* TERION_COMPONENTS_DIFF_VIEW_H */
