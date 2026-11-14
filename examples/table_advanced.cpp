/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <terion/terion.h>

using namespace terion;

int main()
{
    Application app(std::make_unique<AnsiTerminalBackend>());
    State<std::string> selected{"node-7"};
    State<int> horizontal{0};
    State<bool> descending{false};
    State<int> sort_column{0};

    app.render([&] {
        const bool reverse = descending.get();
        const auto chosen = selected.get();
        const int offset = horizontal.get();
        const int sorted = sort_column.get();
        const auto record = [reverse](int index) {
            const int id = reverse ? 99999-index : index;
            const std::string status = id % 17 == 0 ? "FAIL" : id % 4 == 0 ? "WARN" : "OK";
            return TableRow{.key = "node-"+std::to_string(id),
                .cells = {std::to_string(id),"Node-"+std::to_string(id),status,
                    std::to_string((id*7)%100)+"%","Streaming telemetry / node status"},
                .disabled = id % 101 == 0};
        };
        return Paper({.children = {
            Typography({.text = "Terion | #135d Advanced Tables",.variant = TypographyVariant::H2}),
            Typography({.text = "VirtualTable: 100,000 lazy records, RGB cells, stable selection, horizontal scroll"}),
            Row({.children = {
                Button({.text = "First",.on_click = [horizontal]() mutable { horizontal.set(0); }}),
                Button({.text = "Scroll +",.on_click = [horizontal,offset]() mutable {
                    horizontal.set(offset+12);
                }}),
                Button({.text = "Reverse",.on_click = [descending,reverse]() mutable {
                    descending.set(!reverse);
                }})
            },.layout = {.gap = 1}}),
            VirtualTable({.columns = {{"ID",12},{"Node",20},{"State",12},
                    {"Load",10},{"Description",38}},
                .row_count = 100000,
                .row_at = record,
                .row_key_at = [reverse](int index) {
                    return "node-"+std::to_string(reverse ? 99999-index : index);
                },
                .format_cell = [](const TableRow& row,std::size_t index) {
                    TableCellFormat format{.text = row.cells[index]};
                    if (index == 2)
                    {
                        if (format.text == "OK") format.foreground = Color::rgb(80,210,140);
                        else if (format.text == "WARN") format.foreground = Color::rgb(250,192,70);
                        else format.foreground = Color::rgb(255,90,95);
                        format.bold = true;
                    }
                    if (index == 3 && row.key.size() % 2 == 0)
                        format.background = Color::rgb(26,42,66);
                    return format;
                },
                .selected_key = chosen,
                .sort_column = sorted,.sort_ascending = !reverse,
                .on_select_key = [selected](std::string key) mutable { selected.set(std::move(key)); },
                .on_sort = [descending,sort_column,reverse](int column,bool) mutable {
                    sort_column.set(column);
                    descending.set(!reverse);
                },
                .horizontal_offset = offset,
                .on_horizontal_scroll = [horizontal](int next) mutable { horizontal.set(next); },
                .key = "virtual-table",.layout = {.width = 72,.height = 11}}),
            Typography({.text = "Selected: "+chosen+" | offset: "+std::to_string(offset)}),
            Typography({.text = "Table: Shift+Left/Right, Shift+PageUp/Down, Shift+Home/End"}),
            Table({.columns = {{"Name",20},{"Priority",14},{"Description",36}},
                .rows = {{"one",{"Alpha","High","First task: verify data"}},
                    {"two",{"Beta","Low","Second task: check rendering"}},
                    {"three",{"Gamma","Medium","Third task: inspect selection"}}},
                .format_cell = [](const TableRow& row,std::size_t index) {
                    TableCellFormat format{.text = row.cells[index]};
                    if (index == 1) format.foreground = Color::rgb(220,176,80);
                    return format;
                },
                .value = "one",.key = "standard-table",.layout = {.width = 72,.height = 6}}),
            Typography({.text = "Tab: focus | Mouse wheel: vertical | Shift+wheel: horizontal | Esc: quit"})
        },.layout = {.padding = 1,.gap = 1}});
    });
    return app.run();
}
