/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#ifndef TERION_COMPONENTS_BREADCRUMBS_H
#define TERION_COMPONENTS_BREADCRUMBS_H

#include <cstddef>
#include <functional>
#include <string>
#include <utility>
#include <vector>
#include <terion/components/row.h>
#include <terion/components/button.h>
#include <terion/components/typography.h>

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
        LayoutStyle layout = {.gap = 0};
        std::function<void(std::string)> on_navigate = {};
    };

    explicit Breadcrumbs(Props props) : Component(build(std::move(props))) {}

private:
    static Component build(Props props)
    {
        std::vector<Component> entries;
        entries.reserve(props.items.size()*2);
        for (std::size_t i=0; i<props.items.size(); ++i)
        {
            const auto& item=props.items[i];
            if (i!=0)
                entries.emplace_back(Typography({.text=props.separator,
                    .key="separator-"+std::to_string(i)}));
            if (i+1==props.items.size())
                entries.emplace_back(Typography({.text=item.label,.key="current" ,
                    .variant=TypographyVariant::Subtitle}));
            else
                entries.emplace_back(Button({.text=item.label,.key="crumb-"+std::to_string(i),
                    .variant=ButtonVariant::Text,.disabled=item.disabled,
                    .on_click=[callback=props.on_navigate,value=item.value] {
                        if (callback) callback(value);
                    },.size=ButtonSize::Compact}));
        }
        return Row({.children=std::move(entries),.key=std::move(props.key),.layout=props.layout});
    }
}; /* End of class Breadcrumbs */

} /* End of namespace terion */

#endif /* TERION_COMPONENTS_BREADCRUMBS_H */
