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
    std::vector<TableRow> rows;
    for (int i = 1; i <= 30; ++i)
        rows.push_back({.key = "node-"+std::to_string(i),
            .cells = {"Node "+std::to_string(i),i % 3 == 0 ? "WARN" : "OK",
                "10.0.0."+std::to_string(i)}});
    app.render([&] {
        return Paper({.children = {
            Typography({.text = "Table — arrows, PgUp/PgDn, Home/End, mouse",
                .variant = TypographyVariant::H2}),
            Table({.columns = {{"Name",20},{"Status",12},{"Address",18}},
                .rows = rows,.value = selected.get(),.key = "nodes",
                .layout = {.height = 12},
                .onChange = [&](std::string value) { selected.set(std::move(value)); }}),
            Typography({.text = "Selected: "+selected.get()})
        },.layout = {.padding = 1,.gap = 1}});
    });
    return app.run();
}
