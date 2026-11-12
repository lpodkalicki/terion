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
    auto buffer = std::make_shared<LogBuffer>(10000);
    for (int index = 0; index < 10000; ++index)
        buffer->append({.timestamp = "12:00:00",
            .level = index % 37 == 0 ? LogLevel::Error : index % 11 == 0 ?
                LogLevel::Warning : LogLevel::Info,
            .message = "Received packet #"+std::to_string(index)});
    State<int> selected{-1};
    app.render([&] {
        return Paper({.children = {
            Typography({.text = "LogView: 10,000 entries; virtualized styled rows",
                .variant = TypographyVariant::H2}),
            Typography({.text = "Arrow keys, PageUp/PageDown, mouse wheel, scrollbar drag"}),
            LogView({.buffer = buffer,.min_level = LogLevel::Trace,
                .selected_index = selected.get(),.follow_tail = true,
                .on_select = [&](int index) { selected.set(index); },
                .key = "log-view",.layout = {.width = 76,.height = 16}}),
            Typography({.text = "Selected visible row: "+std::to_string(selected.get())})
        },.layout = {.padding = 1,.gap = 1}});
    });
    return app.run();
}
