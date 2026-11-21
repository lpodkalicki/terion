/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#ifndef TERION_COMPONENTS_BREADCRUMBS_H
#define TERION_COMPONENTS_BREADCRUMBS_H

#include <algorithm>
#include <cstddef>
#include <functional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include <terion/components/row.h>
#include <terion/components/button.h>
#include <terion/components/dropdown_menu.h>
#include <terion/components/typography.h>
#include <terion/grapheme.h>

namespace terion {

class Breadcrumbs : public Component
{
public:
    struct Item
    {
        std::string label;
        std::string value;
        bool disabled = false;
    };

    struct Props
    {
        std::vector<Item> items = {};
        std::string separator = " / ";
        std::string key = {};
        std::string id = {};
        LayoutStyle layout = {.gap = 0};
        std::function<void(std::string)> on_navigate = {};
        // 0: show all; >= 3: first item, overflow menu, last N items.
        std::size_t max_visible_items = 0;
        // 0: full labels; otherwise truncate to terminal display columns.
        int max_label_width = 0;
    };

    explicit Breadcrumbs(Props props) : Component(build(std::move(props))) {}

    [[nodiscard]] static std::string abbreviate(const std::string_view value,const int columns)
    {
        if (columns <= 0 || displayWidth(value) <= columns) return std::string(value);
        if (columns == 1) return "…";
        std::string clipped;
        int used = 0;
        for (std::size_t index = 0; index < value.size();)
        {
            const auto grapheme = nextGrapheme(value,index);
            if (!grapheme.bytes || used + grapheme.columns > columns-1) break;
            clipped.append(value.substr(index,grapheme.bytes));
            used += grapheme.columns;
            index += grapheme.bytes;
        }
        return clipped+"…";
    }

private:
    static Component build(Props props)
    {
        std::vector<Component> entries;
        const std::size_t count=props.items.size();
        entries.reserve(count*2);
        if (count == 0) return Row({.key=std::move(props.key),.id = std::move(props.id),.layout=props.layout});
        const bool collapsed=props.max_visible_items >= 3 && count > props.max_visible_items;
        const std::size_t trailing=collapsed ? props.max_visible_items-2 : 0;
        const std::size_t hidden_end=collapsed ? count-trailing : 1;
        const auto separator=[&entries,&props](const std::size_t i) {
            if (!entries.empty()) entries.emplace_back(Typography({.text=props.separator,
                .key="separator-"+std::to_string(i)}));
        };
        const auto add_item=[&](const std::size_t i) {
            const auto& item=props.items[i];
            separator(i);
            const auto label=abbreviate(item.label,props.max_label_width);
            if (i+1 == count)
                entries.emplace_back(Typography({.text=label,.key="current",
                    .variant=TypographyVariant::Subtitle}));
            else
                entries.emplace_back(Button({.text=label,.key="crumb-"+std::to_string(i),
                    .variant=ButtonVariant::Text,.disabled=item.disabled,
                    .on_click=[callback=props.on_navigate,value=item.value] {
                        if (callback) callback(value);
                    },.size=ControlSize::Compact}));
        };
        if (!collapsed)
        {
            for (std::size_t i=0; i<count; ++i) add_item(i);
        }
        else
        {
            add_item(0);
            separator(1);
            std::vector<SelectOption> hidden;
            hidden.reserve(hidden_end-1);
            for (std::size_t i=1; i<hidden_end; ++i)
            {
                const auto& item=props.items[i];
                hidden.push_back({.label=item.label,.value=item.value,.disabled=item.disabled});
            }
            entries.emplace_back(DropdownMenu({.label="…",.options=std::move(hidden),
                .key="overflow",.variant=ButtonVariant::Text,.on_select=props.on_navigate,
                .size=ControlSize::Compact}));
            for (std::size_t i=hidden_end; i<count; ++i) add_item(i);
        }
        return Row({.children=std::move(entries),.key=std::move(props.key),.id = std::move(props.id),.layout=props.layout});
    }
}; /* End of class Breadcrumbs */

} /* End of namespace terion */

#endif /* TERION_COMPONENTS_BREADCRUMBS_H */
