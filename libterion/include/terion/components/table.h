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
        std::string value = {};
        std::string key = {};
        bool disabled = false;
        LayoutStyle layout = {};
        std::function<void(std::string)> onChange = {};
        std::function<void(ComponentEvent&)> onEventCapture = {};
        std::function<void(ComponentEvent&)> onEvent = {};
    }; /* End of struct Props */

    explicit Table(Props props) :
        Component({.type = Type::Table,.key = std::move(props.key),.text = std::move(props.value),
            .layout = props.layout,.on_event_capture = std::move(props.onEventCapture),
            .on_event = std::move(props.onEvent),.on_change = std::move(props.onChange),
            .disabled = props.disabled,.table_columns = std::move(props.columns),
            .table_rows = std::move(props.rows)})
    {}
}; /* End of class Table */

} /* End of namespace terion */

#endif /* TERION_COMPONENTS_TABLE_H */
