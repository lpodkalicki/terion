/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#ifndef TERION_COMPONENTS_COMMAND_PALETTE_H
#define TERION_COMPONENTS_COMMAND_PALETTE_H

#include <algorithm>
#include <functional>
#include <string>
#include <utility>
#include <vector>
#include <terion/components/autocomplete.h>
#include <terion/components/dialog.h>
#include <terion/components/typography.h>

namespace terion {

struct CommandPaletteAction
{
    std::string id;
    std::string title;
    std::string description = {};
    std::string shortcut = {};
    bool disabled = false;
}; /* End of struct CommandPaletteAction */

// A controlled command palette. The caller owns open/query state and can
// register an application-level shortcut with Application::addShortcut().
// Dialog supplies the modal focus scope, while Autocomplete supplies the
// searchable dropdown and keyboard/mouse selection.
class CommandPalette : public Component
{
public:
    struct Props
    {
        bool open = false;
        std::vector<CommandPaletteAction> commands = {};
        std::string query = {};
        std::string title = "Command palette";
        std::string placeholder = "Search commands...";
        std::string key = {};
        int width = 56;
        std::function<void(std::string)> on_query_change = {};
        std::function<void(std::string)> on_execute = {};
        std::function<void()> on_close = {};
    }; /* End of struct Props */

    explicit CommandPalette(Props props) : Component(create(std::move(props))) {}

private:
    static Component create(Props props)
    {
        std::vector<SelectOption> options;
        options.reserve(props.commands.size());
        for (const auto& command : props.commands)
        {
            if (command.id.empty()) continue;
            std::string label = command.title;
            if (!command.description.empty()) label += " - " + command.description;
            if (!command.shortcut.empty()) label += " [" + command.shortcut + "]";
            options.push_back({.label = std::move(label),.value = command.id,
                .disabled = command.disabled});
        }

        const auto on_execute = std::move(props.on_execute);
        const auto on_close = std::move(props.on_close);
        std::vector<Component> children;
        children.reserve(2);
        children.emplace_back(Typography({.text = "Type to find a command; Enter to run; Esc to close"}));
        children.emplace_back(Autocomplete({
            .options = std::move(options),
            .query = std::move(props.query),
            .placeholder = std::move(props.placeholder),
            .key = "command_search",
            .layout = {.width = std::max(12,props.width-6)},
            .on_change = [on_execute,on_close](std::string id) {
                if (on_execute) on_execute(std::move(id));
                if (on_close) on_close();
            },
            .on_input_change = std::move(props.on_query_change)
        }));
        return Dialog({
            .open = props.open,
            .title = std::move(props.title),
            .key = std::move(props.key),
            .width = props.width,
            .on_close = on_close,
            .children = std::move(children),
            .show_actions = false
        });
    }
}; /* End of class CommandPalette */

} /* End of namespace terion */

#endif /* TERION_COMPONENTS_COMMAND_PALETTE_H */
