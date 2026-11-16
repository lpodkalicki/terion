/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */
#include <algorithm>
#include <memory>
#include <string>
#include <terion/terion.h>

using namespace terion;

int main()
{
    Application app(std::make_unique<AnsiTerminalBackend>());
    State<bool> light{false};
    const Color custom = Color::rgb(24,48,74);
    app.render([&] {
        const Size window = app.backend().size();
        const int width = std::max(26,window.width-4);
        const auto success = TextStyleSpec{.foreground = ColorVariant::Success,.bold = true};
        const StyledText decorated = {
            {.text = "  Semantic success   ",.style_spec = success},
            {.text = " RGB background ",.style_spec = TextStyleSpec{
                .foreground = Color::rgb(245,220,130),.background = custom,.underline = true}},
            {.text = "  warning",.style_spec = TextStyleSpec{
                .foreground = ColorVariant::Warning,.italic = true}}
        };
        return ThemeProvider(light.get() ? Theme::light() : Theme::dark(),
            Column({.children = {
                Typography({.text = "#139c | Unified Styling API",.variant = TypographyVariant::H2}),
                Button({.text = light.get() ? "Switch to Dark" : "Switch to Light",
                    .on_click = [&] { light.set(!light.get()); }}),
                Typography({.text = "Semantic color follows the theme",.style_spec = success}),
                Typography({.text = "Explicit RGB remains unchanged",
                    .style_spec = TextStyleSpec{.foreground = Color::rgb(210,95,160),.italic = true}}),
                Divider({.layout = {.width = width},
                    .style_spec = TextStyleSpec{.foreground = ColorVariant::Info}}),
                VirtualList({.item_count = 1,.item_styled = [decorated](int) { return decorated; },
                    .layout = {.width = width,.height = 1},.show_scrollbar = false}),
                Typography({.text = "Cell styling (foreground, background, italic):"}),
                VirtualTable({.columns = {{.title = "Status",.width = 16},
                        {.title = "Description",.width = 22}},.row_count = 2,
                    .row_at = [](int i) { return TableRow{.key = std::to_string(i),
                        .cells = {i == 0 ? "OK" : "WARN", "Paint-time styles"}}; },
                    .format_cell = [custom](const TableRow& row,std::size_t col) {
                        if (col != 0) return TableCellFormat{.text = row.cells[col]};
                        return TableCellFormat{.text = row.cells[col],
                            .style_spec = TextStyleSpec{.foreground = row.key == "0" ?
                                ColorSpec{ColorVariant::Success} : ColorSpec{ColorVariant::Warning},
                                .background = custom,.bold = true}};
                    },.layout = {.width = width,.height = 4},.show_scrollbar = false}),
                Typography({.text = "Styles are resolved when painted; change the theme above."}),
                Typography({.text = "Tab to focus the button; Enter to toggle, Esc to exit."})
            },.layout = {.width = window.width,.height = window.height,.padding = 1,.gap = 1}}));
    });
    return app.run();
}
