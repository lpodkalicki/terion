/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <terion/terion.h>

using namespace terion;

int main()
{
    Application app(std::make_unique<AnsiTerminalBackend>());
    State<std::string> city("lublin");
    State<std::string> query("");
    const std::vector<SelectOption> cities = {
        {.label = "Warszawa",.value = "warsaw"},
        {.label = "Kraków",.value = "krakow"},
        {.label = "Lublin",.value = "lublin"},
        {.label = "Gdańsk",.value = "gdansk"},
        {.label = "Wrocław",.value = "wroclaw"},
        {.label = "Poznań",.value = "poznan"},
        {.label = "Unavailable",.value = "unavailable",.disabled = true}
    };
    app.render([&] {
        return Column({.children = {
            Typography({.text = "Autocomplete - type to filter, arrows/Enter to select, Esc to close"}),
            Autocomplete({.options = cities,.value = city.get(),.query = query.get(),
                .placeholder = "Search city...",.key = "city",.layout = {.width = 30},
                .on_change = [&](std::string value) { city.set(std::move(value)); },
                .on_input_change = [&](std::string value) { query.set(std::move(value)); }}),
            Typography({.text = "Selected: " + city.get() + " | Search: " + query.get()})
        },.layout = {.padding = 2,.gap = 1}});
    });
    return app.run();
}
