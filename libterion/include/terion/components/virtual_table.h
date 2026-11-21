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
#include <terion/components/table_view_detail.h>
#include <terion/grapheme.h>

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
        // Optional O(1) lookup for stable selection without reading a full record.
        std::function<std::string(int)> row_key_at = {};
        std::function<TableCellFormat(const TableRow&,std::size_t)> format_cell = {};
        int selected_index = -1;
        std::string selected_key = {}; // Takes precedence over selected_index when nonempty.
        int sort_column = -1;
        bool sort_ascending = true;
        std::function<void(int)> on_selection_change = {}; // Index-based selection.
        std::function<void(std::string)> on_selection_key_change = {}; // Stable key selection.
        std::function<void(int,bool)> on_sort = {};
        int horizontal_offset = 0;
        std::function<void(int)> on_horizontal_scroll = {};
        std::string key = {};
        std::string id = {};
        LayoutStyle layout = {};
        bool disabled = false;
        bool show_scrollbar = true;
    }; /* End of struct Props */

    explicit VirtualTable(Props props) : Component(create(std::move(props))) {}

private:
    static std::string clip(const std::string_view text, const int max_width)
    {
        std::size_t end = 0;
        int width = 0;
        while (end < text.size())
        {
            const auto cluster = nextGrapheme(text,end);
            if (!cluster.bytes || width + cluster.columns > max_width) break;
            width += cluster.columns;
            end += cluster.bytes;
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
        const std::string selected_key = std::move(props.selected_key);
        const int sort_column = props.sort_column;
        const bool sort_ascending = props.sort_ascending;
        const bool show_scrollbar = props.show_scrollbar;
        const auto row_at = std::move(props.row_at);
        const auto row_key_at = std::move(props.row_key_at);
        const auto format_cell = std::move(props.format_cell);
        const auto on_select = std::move(props.on_selection_change);
        const auto on_select_key = std::move(props.on_selection_key_change);
        const auto on_sort = std::move(props.on_sort);
        const auto on_horizontal_scroll = std::move(props.on_horizontal_scroll);

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
        const int visible_width = std::max(0,table_width-(show_scrollbar ? 1 : 0));
        const bool scroll_controls = static_cast<bool>(on_horizontal_scroll) &&
            calculated_width > visible_width && visible_width >= 8;
        const int plot_width = std::max(0,visible_width-(scroll_controls ? 4 : 0));
        const int max_offset = std::max(0,calculated_width-plot_width);
        const int offset = std::clamp(props.horizontal_offset,0,max_offset);

        std::vector<Component> headings;
        headings.reserve(props.columns.size()+(scroll_controls ? 2U : 0U));
        int column_start = 0;
        for (std::size_t index = 0; index < props.columns.size(); ++index)
        {
            const auto& column = props.columns[index];
            const int width = widths[index];
            const int left = std::max(column_start,offset);
            const int right = std::min(column_start+width,offset+plot_width);
            if (right > left)
            {
                std::string title = column.title;
                if (static_cast<int>(index) == sort_column)
                    title += sort_ascending ? " ^" : " v";
                const int column_index = static_cast<int>(index);
                const std::string aligned = alignCell(title,std::max(0,width-2),column.alignment)+"  ";
                headings.emplace_back(Button({
                    .text = detail::sliceColumns(aligned,left-column_start,right-left),
                    .key = "header_"+std::to_string(index),
                    .variant = ButtonVariant::Text,
                    .disabled = disabled || !on_sort,
                    .on_click = [on_sort,column_index,sort_column,sort_ascending] {
                        if (on_sort) on_sort(column_index,column_index == sort_column ? !sort_ascending : true);
                    },
                    .layout = {.width = right-left,.height = 1},
                    .size = ControlSize::Compact
                }));
            }
            column_start += width;
            if (column_start >= offset+plot_width) break;
        }
        if (scroll_controls)
        {
            const int step = std::max(1,plot_width/2);
            headings.emplace_back(Button({.text = "<",.key = "horizontal_previous",
                .variant = ButtonVariant::Text,.disabled = disabled || offset == 0,
                .on_click = [on_horizontal_scroll,offset,step] {
                    on_horizontal_scroll(std::max(0,offset-step));
                },.layout = {.width = 2,.height = 1},.size = ControlSize::Compact}));
            headings.emplace_back(Button({.text = ">",.key = "horizontal_next",
                .variant = ButtonVariant::Text,.disabled = disabled || offset >= max_offset,
                .on_click = [on_horizontal_scroll,offset,step,max_offset] {
                    on_horizontal_scroll(std::min(max_offset,offset+step));
                },.layout = {.width = 2,.height = 1},.size = ControlSize::Compact}));
        }

        const auto styled_row = [row_at,format_cell,columns = std::move(props.columns),widths,
            offset,plot_width](int index) {
            StyledText full;
            full.reserve(widths.size());
            const TableRow row = row_at ? row_at(index) : TableRow{};
            for (std::size_t col = 0; col < widths.size(); ++col)
            {
                const auto format = format_cell ? format_cell(row,col) : TableCellFormat{
                    .text = col < row.cells.size() ? row.cells[col] : std::string{}};
                full.push_back({.text = alignCell(format.text,widths[col]-1,columns[col].alignment)+" ",
                    .bold = format.bold,.foreground = format.foreground,
                    .background_color = format.background,.style_spec = format.style_spec});
            }
            return detail::sliceStyledColumns(full,offset,plot_width);
        };
        const auto select_row = [row_at,row_key_at,on_select,on_select_key,disabled](int index) {
            if (disabled || (!on_select && !on_select_key)) return;
            // Fetch only the selected row, never scan the table.
            const TableRow row = row_at ? row_at(index) : TableRow{};
            if (row.disabled) return;
            if (on_select) on_select(index);
            if (on_select_key)
            {
                const std::string key = row_key_at ? row_key_at(index) : row.key;
                if (!key.empty()) on_select_key(key);
            }
        };
        const auto selected_by_key = [selected_key,row_key_at,row_at](int index) {
            if (selected_key.empty()) return false;
            return (row_key_at ? row_key_at(index) : row_at ? row_at(index).key : std::string{}) == selected_key;
        };
        std::vector<Component> children;
        children.reserve(2);
        children.emplace_back(Row({.children = std::move(headings),.key = "headings",
            .layout = {.width = table_width,.height = 1,.clip = true}}));
        children.emplace_back(VirtualList({.item_count = count,
            .item_styled = styled_row,
            .selected_index = selected_index,
            .item_is_selected = selected_key.empty() ? std::function<bool(int)>{} : selected_by_key,
            .on_selection_change = select_row,
            .key = "rows",.id = props.id,
            .layout = {.width = table_width,.height = std::max(0,requested_height-1)},
            .disabled = disabled,
            .show_scrollbar = show_scrollbar}));
        return Column({.children = std::move(children),.key = std::move(props.key),.id = std::move(props.id),
            .layout = {.width = table_width,.height = std::max(1,requested_height),.clip = true}});
    }
}; /* End of class VirtualTable */

} /* End of namespace terion */

#endif /* TERION_COMPONENTS_VIRTUAL_TABLE_H */
