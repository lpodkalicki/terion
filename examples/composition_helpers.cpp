/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <terion/terion.h>

using namespace terion;

struct Item
{
    std::string id;
    std::string title;
};

const auto ItemRow = component([](const Item& item) {
    return Typography({.text = item.title});
});

int main()
{
    Application app(std::make_unique<AnsiTerminalBackend>());
    State<bool> expanded(true);
    const std::vector<Item> items{{"one","First"},{"two","Second"}};

    app.render([&] {
        return Column({.children = {
            Button({.text = "Toggle list",.onClick = [&] { expanded.set(!expanded.get()); }}),
            When(expanded.get(),ForEach(items,[](const Item& item) {
                return ItemRow(item).keyed(item.id);
            }))
        },.layout = {.padding = 2,.gap = 1}});
    });
    return app.run();
}
