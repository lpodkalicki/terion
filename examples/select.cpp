/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <terion/terion.h>

using namespace terion;

int main()
{
    Application app(std::make_unique<AnsiTerminalBackend>());
    State<std::string> selected("pl");
    State<std::string> city("lublin");
    app.render([&] {
        return Column({.children = {
            Typography({.text = "Terion Select — Enter/Space, arrows, mouse, wheel, Esc"}),
            Select({.options = {
                {.label = "Polski",.value = "pl"},
                {.label = "English",.value = "en"},
                {.label = "Deutsch",.value = "de"},
                {.label = "Français",.value = "fr"},
                {.label = "Español",.value = "es"},
                {.label = "Italiano",.value = "it"},
                {.label = "Português",.value = "pt"},
                {.label = "Unavailable",.value = "disabled",.disabled = true}
            },.value = selected.get(),.placeholder = "Select language",.key = "language",
                .layout = {.width = 28},.onChange = [&](std::string value) { selected.set(std::move(value)); }}),
            Select({.options = {
                {.label = "Warszawa",.value = "warsaw"},
                {.label = "Kraków",.value = "krakow"},
                {.label = "Gdańsk",.value = "gdansk"},
                {.label = "Wrocław",.value = "wroclaw"},
                {.label = "Poznań",.value = "poznan"},
                {.label = "Łódź",.value = "lodz"},
                {.label = "Katowice",.value = "katowice"},
                {.label = "Lublin",.value = "lublin"},
                {.label = "Rzeszów",.value = "rzeszow"},
                {.label = "Szczecin",.value = "szczecin"},
                {.label = "Białystok",.value = "bialystok"},
                {.label = "Olsztyn",.value = "olsztyn"}
            },.value = city.get(),.placeholder = "Select city",.key = "city",
                .layout = {.width = 28},.onChange = [&](std::string value) { city.set(std::move(value)); }}),
            Typography({.text = "Language: " + selected.get() + " | City: " + city.get()}),
            Typography({.text = "Open each Select to verify OverlayManager closes the other."})
        },.layout = {.padding = 2,.gap = 1}});
    });
    return app.run();
}
