/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#ifndef TERION_COMPONENTS_ACCORDION_H
#define TERION_COMPONENTS_ACCORDION_H

#include <algorithm>
#include <functional>
#include <string>
#include <utility>
#include <vector>
#include <terion/components/column.h>
#include <terion/components/button.h>
#include <terion/components/box.h>
#include <terion/components/tab_panel.h>

namespace terion {

class Accordion : public Component
{
public:
    struct Item
    {
        std::string id;
        std::string title;
        std::vector<Component> children = {};
        bool disabled = false;
        std::string description = {};
    };

    struct Props
    {
        std::vector<Item> items = {};
        std::vector<std::string> expanded = {};
        bool multiple = false;
        std::string key = {};
        LayoutStyle layout = {.gap = 0};
        std::function<void(std::vector<std::string>)> on_change = {};
        // Keep content reconciled, but outside layout, painting, hit-testing and focus.
        bool keep_mounted = false;
    };

    explicit Accordion(Props props) : Component(build(std::move(props))) {}

    [[nodiscard]] static std::vector<std::string> toggle(const std::vector<std::string>& expanded,
        const std::string& id,bool multiple)
    {
        const bool open=std::find(expanded.begin(),expanded.end(),id)!=expanded.end();
        if (!multiple) return open?std::vector<std::string>{}:std::vector<std::string>{id};
        auto result=expanded;
        if (open) result.erase(std::remove(result.begin(),result.end(),id),result.end());
        else result.push_back(id);
        return result;
    }

    [[nodiscard]] static std::vector<std::string> expandAll(const std::vector<Item>& items)
    {
        std::vector<std::string> ids;
        ids.reserve(items.size());
        for (const auto& item : items)
            if (!item.disabled) ids.push_back(item.id);
        return ids;
    }

    [[nodiscard]] static std::vector<std::string> collapseAll() { return {}; }

private:
    static Component build(Props props)
    {
        std::vector<Component> sections;
        sections.reserve(props.items.size());
        for (auto& item : props.items)
        {
            const bool open=std::find(props.expanded.begin(),props.expanded.end(),item.id)!=props.expanded.end();
            const auto id=item.id;
            std::vector<Component> section;
            section.emplace_back(Button({.text=(open?"▾ ":"▸ ")+item.title+
                    (item.description.empty() ? "" : " — "+item.description),
                .key="header",.variant=ButtonVariant::Text,.disabled=item.disabled,
                .on_click=[expanded=props.expanded,id,multiple=props.multiple,
                    callback=props.on_change] {if (callback) callback(toggle(expanded,id,multiple));},
                .size=ButtonSize::Compact}));
            if (props.keep_mounted)
                section.emplace_back(TabPanel({.children={Box({.children=std::move(item.children),
                        .key="body",.layout={.padding=1,.gap=1}})},
                    .key="content",.active=open}));
            else if (open)
                section.emplace_back(Box({.children=std::move(item.children),.key="content",
                    .layout={.padding=1,.gap=1}}));
            sections.emplace_back(Column({.children=std::move(section),.key="section-"+item.id}));
        }
        return Column({.children=std::move(sections),.key=std::move(props.key),.layout=props.layout});
    }
}; /* End of class Accordion */

} /* End of namespace terion */

#endif /* TERION_COMPONENTS_ACCORDION_H */
