/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#ifndef TERION_COMPONENTS_STEPPER_H
#define TERION_COMPONENTS_STEPPER_H

#include <algorithm>
#include <cstddef>
#include <functional>
#include <string>
#include <utility>
#include <vector>
#include <terion/components/column.h>
#include <terion/components/row.h>
#include <terion/components/button.h>
#include <terion/components/typography.h>

namespace terion {

class Stepper : public Component
{
public:
    struct Step
    {
        std::string label;
        bool optional = false;
        bool disabled = false;
    };

    struct Props
    {
        std::vector<Step> steps = {};
        std::size_t active_step = 0;
        bool linear = true;
        bool allow_navigation = true;
        std::string key = {};
        LayoutStyle layout = {.gap = 1};
        std::function<void(std::size_t)> on_change = {};
    };

    explicit Stepper(Props props) : Component(build(std::move(props))) {}

private:
    static Component build(Props props)
    {
        const auto current = props.steps.empty() ? 0 : std::min(props.active_step,props.steps.size()-1);
        std::vector<Component> entries;
        entries.reserve(props.steps.size()*2);
        for (std::size_t i=0; i<props.steps.size(); ++i)
        {
            const auto& step=props.steps[i];
            const bool done=i<current;
            const bool blocked=step.disabled || !props.allow_navigation ||
                (props.linear && i>current);
            const std::string prefix=done?"[x] ":(i==current?"[>] ":"[ ] ");
            entries.emplace_back(Button({.text=prefix+step.label+(step.optional?" (optional)":""),
                .key="step-"+std::to_string(i),
                .variant=i==current?ButtonVariant::Contained:ButtonVariant::Text,
                .color=done?ColorVariant::Success:ColorVariant::Primary,
                .disabled=blocked || i==current,
                .on_click=[callback=props.on_change,i] {if (callback) callback(i);},
                .size=ButtonSize::Compact}));
            if (i+1<props.steps.size())
                entries.emplace_back(Typography({.text="›",.key="separator-"+std::to_string(i)}));
        }
        return Row({.children=std::move(entries),.key=std::move(props.key),.layout=props.layout});
    }
}; /* End of class Stepper */

} /* End of namespace terion */

#endif /* TERION_COMPONENTS_STEPPER_H */
