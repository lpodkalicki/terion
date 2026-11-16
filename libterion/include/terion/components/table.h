/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#ifndef TERION_COMPONENTS_TABLE_H
#define TERION_COMPONENTS_TABLE_H

#include <terion/component.h>

namespace terion {

class Table : public Component
{
public:
    struct Props
    {
        std::vector<TableColumn> columns = {};
        std::vector<TableRow> rows = {};
        std::function<bool(const TableRow&)> filter = {};
        std::function<TableCellFormat(const TableRow&,std::size_t)> format_cell = {};
        std::string value = {};
        std::string key = {};
        bool disabled = false;
        LayoutStyle layout = {};
        int horizontal_offset = -1;
        std::function<void(std::string)> on_change = {};
        std::function<void(int)> on_horizontal_scroll = {};
        std::function<void(ComponentEvent&)> on_event_capture = {};
        std::function<void(ComponentEvent&)> on_event = {};
        // Prefer these names for keyed collection selection. Legacy value/on_change remain supported.
        std::optional<std::string> selected_key = std::nullopt;
        std::function<void(std::string)> on_selection_change = {};
    }; /* End of struct Props */

    explicit Table(Props props) :
        Component({.type = Type::Table,.key = std::move(props.key),.text = props.selected_key.value_or(props.value),
            .layout = props.layout,.on_event_capture = std::move(props.on_event_capture),
            .on_event = std::move(props.on_event),.on_change = props.on_selection_change ? std::move(props.on_selection_change) :
                std::move(props.on_change),
            .disabled = props.disabled,.table_columns = std::move(props.columns),
            .table_rows = std::move(props.rows),.table_filter = std::move(props.filter),
            .table_format_cell = std::move(props.format_cell),
            .table_horizontal_offset = props.horizontal_offset,
            .table_on_horizontal_scroll = std::move(props.on_horizontal_scroll)})
    {}
}; /* End of class Table */

} /* End of namespace terion */

#endif /* TERION_COMPONENTS_TABLE_H */
