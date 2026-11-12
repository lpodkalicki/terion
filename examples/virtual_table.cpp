/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <memory>
#include <string>
#include <terion/terion.h>

using namespace terion;

int main()
{
    Application app(std::make_unique<AnsiTerminalBackend>());
    State<int> selected{-1};
    State<int> sort_column{-1};
    State<bool> sort_ascending{true};
    constexpr int row_count = 100000;

    app.render([&] {
        return Column({.children = {
            Typography({.text = "VirtualTable: 100,000 lazy rows",.variant = TypographyVariant::H2}),
            Typography({.text = "Click headings to request sort, arrows to select, wheel to scroll"}),
            VirtualTable({
                .columns = {{.title = "ID",.width = 12,.alignment = TableAlignment::Right},
                    {.title = "Name",.width = 24}, {.title = "Status",.width = 16}},
                .row_count = row_count,
                .row_at = [](int index) {
                    return TableRow{.key = std::to_string(index),
                        .cells = {std::to_string(index+1),"Station "+std::to_string(index+1),
                            index%2 == 0 ? "ONLINE" : "OFFLINE"}};
                },
                .selected_index = selected.get(),
                .sort_column = sort_column.get(),
                .sort_ascending = sort_ascending.get(),
                .on_select = [&](int index) { selected.set(index); },
                .on_sort = [&](int column, bool ascending) {
                    sort_column.set(column);
                    sort_ascending.set(ascending);
                    // The data provider decides how to apply the requested ordering.
                },
                .key = "table",
                .layout = {.width = 54,.height = 15}
            }),
            Typography({.text = "Selected index: "+std::to_string(selected.get())})
        },.layout = {.padding = 1,.gap = 1}});
    });
    return app.run();
}
