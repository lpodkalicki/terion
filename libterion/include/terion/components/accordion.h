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
    };

    struct Props
    {
        std::vector<Item> items = {};
        std::vector<std::string> expanded = {};
        bool multiple = false;
        std::string key = {};
        LayoutStyle layout = {.gap = 0};
        std::function<void(std::vector<std::string>)> on_change = {};
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

private:
    static Component build(Props props)
    {
        std::vector<Component> sections;
        sections.reserve(props.items.size());
        for (auto& item : props.items)
        {
            const bool open=std::find(props.expanded.begin(),props.expanded.end(),item.id)!=props.expanded.end();
            auto id=item.id;
            std::vector<Component> section;
            section.emplace_back(Button({.text=(open?"▾ ":"▸ ")+item.title,
                .key="header",.variant=ButtonVariant::Text,.disabled=item.disabled,
                .on_click=[expanded=props.expanded,id=std::move(id),multiple=props.multiple,
                    callback=props.on_change] {if (callback) callback(toggle(expanded,id,multiple));},
                .size=ButtonSize::Compact}));
            if (open)
                section.emplace_back(Box({.children=std::move(item.children),.key="content",
                    .layout={.padding=1,.gap=1}}));
            sections.emplace_back(Column({.children=std::move(section),.key="section-"+item.id}));
        }
        return Column({.children=std::move(sections),.key=std::move(props.key),.layout=props.layout});
    }
}; /* End of class Accordion */

} /* End of namespace terion */

#endif /* TERION_COMPONENTS_ACCORDION_H */
