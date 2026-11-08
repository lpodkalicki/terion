/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <memory>
#include <terion/terion.h>

using namespace terion;

int main()
{
    Application app(std::make_unique<AnsiTerminalBackend>());
    State<std::string> selected{"node-1"};
    State<std::string> filter{""};
    std::vector<TableRow> rows;
    for (int i = 1; i <= 30; ++i)
        rows.push_back({.key = "node-"+std::to_string(i),
            .cells = {"Node "+std::to_string(i),i % 3 == 0 ? "WARN" : "OK",
                "10.0.0."+std::to_string(i)}});
    app.render([&] {
        return Paper({.children = {
            Typography({.text = "Table — click header to sort, drag scrollbar, wheel, arrows, PgUp/PgDn",
                .variant = TypographyVariant::H2}),
            TextField({.value = filter.get(),.placeholder = "Filter nodes...",.key = "filter",
                .onChange = [&](std::string value) { filter.set(std::move(value)); }}),
            Table({.columns = {{"Name",20},{"Status",12,TableAlignment::Center},
                    {"Address",18,TableAlignment::Right}},
                .rows = rows,
                .filter = [query = filter.get()](const TableRow& row) {
                    return query.empty() || row.cells[0].find(query) != std::string::npos ||
                        row.cells[1].find(query) != std::string::npos;
                },
                .formatCell = [](const TableRow& row, std::size_t column) {
                    TableCellFormat cell {.text = row.cells[column]};
                    if (column == 1)
                    {
                        cell.foreground = row.cells[column] == "OK" ?
                            Color::rgb(70,210,120) : Color::rgb(255,185,65);
                        cell.bold = true;
                    }
                    return cell;
                },
                .value = selected.get(),.key = "nodes",
                .layout = {.height = 12},
                .onChange = [&](std::string value) { selected.set(std::move(value)); }}),
            Typography({.text = "Selected: "+selected.get()})
        },.layout = {.padding = 1,.gap = 1}});
    });
    return app.run();
}
