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
    State<int> selected{0};
    constexpr int item_count = 100000;

    app.render([&] {
        return Paper({.children = {
            Typography({.text = "VirtualList: 100,000 rows, constant-size retained tree",
                .variant = TypographyVariant::H2}),
            Typography({.text = "Arrows, Home/End, PgUp/PgDn, mouse wheel and scrollbar drag"}),
            VirtualList({.item_count = item_count,
                .item_text = [](int index) { return "Record "+std::to_string(index+1); },
                .selected_index = selected.get(),
                .on_select = [&](int index) { selected.set(index); },
                .key = "virtual-list",.layout = {.width = 50,.height = 14}}),
            Typography({.text = "Selected record: "+std::to_string(selected.get()+1)})
        },.layout = {.padding = 1,.gap = 1}});
    });
    return app.run();
}
