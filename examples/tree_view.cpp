/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <algorithm>
#include <memory>
#include <terion/terion.h>

using namespace terion;

int main()
{
    Application app(std::make_unique<AnsiTerminalBackend>());
    State<std::string> selected{"node-1"};
    State<std::vector<std::string>> expanded{{"stations","station-a","nodes"}};
    std::vector<TreeItem> nodes;
    for (int i = 1; i <= 20; ++i)
        nodes.push_back({.key = "node-"+std::to_string(i),
            .label = "Node "+std::to_string(i),.icon = "●"});
    const std::vector<TreeItem> items = {
        {.key = "stations",.label = "Stations",.children = {
            {.key = "station-a",.label = "Station A",.children = {
                {.key = "nodes",.label = "Nodes",.children = nodes},
                {.key = "services",.label = "Services",.children = {
                    {.key = "receiver",.label = "Receiver"},
                    {.key = "tracker",.label = "Tracker"}}}}},
            {.key = "station-b",.label = "Station B",.children = {
                {.key = "standby",.label = "Standby",.disabled = true}}}}},
        {.key = "configuration",.label = "Configuration",.children = {
            {.key = "network",.label = "Network"},
            {.key = "logging",.label = "Logging"}}}
    };
    app.render([&] {
        return Paper({.children = {
            Typography({.text = "TreeView — arrows, Enter/Space, mouse, wheel",
                .variant = TypographyVariant::H2}),
            TreeView({.items = items,.expanded = expanded.get(),.value = selected.get(),
                .key = "tree",.layout = {.width = 38,.height = 14},
                .onChange = [&](std::string value) { selected.set(std::move(value)); },
                .onToggle = [&](std::string key, bool open) {
                    auto values = expanded.get();
                    values.erase(std::remove(values.begin(),values.end(),key),values.end());
                    if (open) values.push_back(std::move(key));
                    expanded.set(std::move(values));
                }}),
            Typography({.text = "Selected: "+selected.get()})
        },.layout = {.padding = 1,.gap = 1}});
    });
    return app.run();
}
