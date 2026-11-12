/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#ifndef TERION_COMPONENTS_VIRTUAL_TABLE_H
#define TERION_COMPONENTS_VIRTUAL_TABLE_H

#include <algorithm>
#include <functional>
#include <limits>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include <terion/components/button.h>
#include <terion/components/column.h>
#include <terion/components/row.h>
#include <terion/components/virtual_list.h>
#include <terion/utf8.h>

namespace terion {

// A fixed-height table header and a virtualized body. Data retrieval is lazy:
// row_at(index) is called only for visible records and an explicit selection.
// Sorting is controlled by the caller (on_sort); this component never scans
// row_count or allocates one component per data row.
class VirtualTable : public Component
{
public:
    struct Props
    {
        std::vector<TableColumn> columns = {};
        int row_count = 0;
        std::function<TableRow(int)> row_at = {};
        std::function<TableCellFormat(const TableRow&,std::size_t)> format_cell = {};
        int selected_index = -1;
        int sort_column = -1;
        bool sort_ascending = true;
        std::function<void(int)> on_select = {};
        std::function<void(int,bool)> on_sort = {};
        std::string key = {};
        LayoutStyle layout = {};
        bool disabled = false;
        bool show_scrollbar = true;
    }; /* End of struct Props */

    explicit VirtualTable(Props props) : Component(create(std::move(props))) {}

private:
    static int displayWidth(const std::string_view text)
    {
        int width = 0;
        for (std::size_t pos = 0; pos < text.size();)
        {
            const auto decoded = decodeUtf8(text,pos);
            if (decoded.bytes == 0) break;
            width += codePointWidth(decoded.code_point);
            pos += decoded.bytes;
        }
        return width;
    }

    static std::string clip(const std::string_view text, const int max_width)
    {
        std::size_t end = 0;
        int width = 0;
        while (end < text.size())
        {
            const auto decoded = decodeUtf8(text,end);
            if (decoded.bytes == 0) break;
            const int glyph_width = codePointWidth(decoded.code_point);
            if (width+glyph_width > max_width) break;
            width += glyph_width;
            end += decoded.bytes;
        }
        return std::string(text.substr(0,end));
    }

    static std::string alignCell(const std::string_view value, const int width,
        const TableAlignment alignment)
    {
        const auto text = clip(value,std::max(0,width));
        const int available = std::max(0,width-displayWidth(text));
        const int left = alignment == TableAlignment::Right ? available :
            alignment == TableAlignment::Center ? available/2 : 0;
        return std::string(static_cast<std::size_t>(left),' ')+text+
            std::string(static_cast<std::size_t>(available-left),' ');
    }

    static Component create(Props props)
    {
        const int count = std::max(0,props.row_count);
        const int requested_width = props.layout.width.value_or(0);
        const int requested_height = props.layout.height.value_or(11);
        const bool disabled = props.disabled;
        const int selected_index = props.selected_index;
        const int sort_column = props.sort_column;
        const bool sort_ascending = props.sort_ascending;
        const bool show_scrollbar = props.show_scrollbar;
        const auto row_at = std::move(props.row_at);
        const auto format_cell = std::move(props.format_cell);
        const auto on_select = std::move(props.on_select);
        const auto on_sort = std::move(props.on_sort);

        std::vector<int> widths;
        widths.reserve(props.columns.size());
        int calculated_width = 0;
        for (const auto& column : props.columns)
        {
            const int width = std::clamp(column.width > 0 ? column.width : displayWidth(column.title)+3,3,10000);
            widths.push_back(width);
            calculated_width = calculated_width > std::numeric_limits<int>::max()-width ?
                std::numeric_limits<int>::max() : calculated_width+width;
        }
        const int table_width = requested_width > 0 ? requested_width :
            std::max(1,calculated_width+(show_scrollbar ? 1 : 0));

        std::vector<Component> headings;
        headings.reserve(props.columns.size());
        for (std::size_t index = 0; index < props.columns.size(); ++index)
        {
            const auto& column = props.columns[index];
            const int width = widths[index];
            std::string title = column.title;
            if (static_cast<int>(index) == sort_column)
                title += sort_ascending ? " ^" : " v";
            const int column_index = static_cast<int>(index);
            headings.emplace_back(Button({
                .text = alignCell(title,std::max(0,width-2),column.alignment),
                .key = "header_"+std::to_string(index),
                .variant = ButtonVariant::Text,
                .disabled = disabled || !on_sort,
                .on_click = [on_sort,column_index,sort_column,sort_ascending] {
                    if (on_sort) on_sort(column_index,column_index == sort_column ? !sort_ascending : true);
                },
                .layout = {.width = width,.height = 1},
                .size = ButtonSize::Compact
            }));
        }

        const auto styled_row = [row_at,format_cell,columns = std::move(props.columns),widths](int index) {
            StyledText result;
            result.reserve(widths.size());
            const TableRow row = row_at ? row_at(index) : TableRow{};
            for (std::size_t col = 0; col < widths.size(); ++col)
            {
                const auto format = format_cell ? format_cell(row,col) : TableCellFormat{
                    .text = col < row.cells.size() ? row.cells[col] : std::string{}};
                result.push_back({.text = alignCell(format.text,widths[col]-1,columns[col].alignment)+" ",
                    .color = ColorVariant::Default,
                    .bold = format.bold});
            }
            return result;
        };
        const auto select_row = [row_at,on_select,disabled](int index) {
            if (disabled || !on_select) return;
            if (row_at && row_at(index).disabled) return;
            on_select(index);
        };
        std::vector<Component> children;
        children.reserve(2);
        children.emplace_back(Row({.children = std::move(headings),.key = "headings",
            .layout = {.width = table_width,.height = 1,.clip = true}}));
        children.emplace_back(VirtualList({.item_count = count,
            .item_styled = styled_row,
            .selected_index = selected_index,
            .on_select = select_row,
            .key = "rows",
            .layout = {.width = table_width,.height = std::max(0,requested_height-1)},
            .disabled = disabled,
            .show_scrollbar = show_scrollbar}));
        return Column({.children = std::move(children),.key = std::move(props.key),
            .layout = {.width = table_width,.height = std::max(1,requested_height),.clip = true}});
    }
}; /* End of class VirtualTable */

} /* End of namespace terion */

#endif /* TERION_COMPONENTS_VIRTUAL_TABLE_H */
