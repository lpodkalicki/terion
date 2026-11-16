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

enum class StepStatus
{
    Auto,
    Pending,
    Active,
    Completed,
    Error,
    Skipped
}; /* End of enum StepStatus */

enum class StepperOrientation
{
    Horizontal,
    Vertical
}; /* End of enum StepperOrientation */

class Stepper : public Component
{
public:
    struct Step
    {
        std::string label;
        bool optional = false;
        bool disabled = false;
        StepStatus status = StepStatus::Auto;
        std::string description = {};
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
        // Advanced step options.
        bool completed = false;
        StepperOrientation orientation = StepperOrientation::Horizontal;
        // Return false to reject a user-initiated transition (e.g. invalid form data).
        std::function<bool(std::size_t,std::size_t)> on_before_change = {};
    };

    explicit Stepper(Props props) : Component(build(std::move(props))) {}

    [[nodiscard]] static StepStatus statusAt(const Props& props,const std::size_t index)
    {
        if (index >= props.steps.size()) return StepStatus::Pending;
        const StepStatus explicit_status = props.steps[index].status;
        if (explicit_status != StepStatus::Auto) return explicit_status;
        if (index < props.active_step || (props.completed && index == props.active_step))
            return StepStatus::Completed;
        if (index == props.active_step) return StepStatus::Active;
        return StepStatus::Pending;
    }

private:
    [[nodiscard]] static std::string prefix(const StepStatus status)
    {
        switch (status)
        {
            case StepStatus::Completed: return "[x] ";
            case StepStatus::Active: return "[>] ";
            case StepStatus::Error: return "[!] ";
            case StepStatus::Skipped: return "[-] ";
            case StepStatus::Auto:
            case StepStatus::Pending: return "[ ] ";
        }
        return "[ ] ";
    }

    [[nodiscard]] static ColorVariant color(const StepStatus status)
    {
        switch (status)
        {
            case StepStatus::Completed: return ColorVariant::Success;
            case StepStatus::Error: return ColorVariant::Error;
            case StepStatus::Skipped: return ColorVariant::Warning;
            default: return ColorVariant::Primary;
        }
    }

    static Component build(Props props)
    {
        const auto current = props.steps.empty() ? 0 : std::min(props.active_step,props.steps.size()-1);
        props.active_step = current;
        std::vector<Component> entries;
        entries.reserve(props.steps.size()*2);
        for (std::size_t i = 0; i < props.steps.size(); ++i)
        {
            const auto& step = props.steps[i];
            const StepStatus status = statusAt(props,i);
            const bool blocked = step.disabled || !props.allow_navigation ||
                (props.linear && i > current);
            std::string text = prefix(status)+step.label+(step.optional ? " (optional)" : "");
            if (props.orientation == StepperOrientation::Vertical && !step.description.empty())
                text += " — "+step.description;
            entries.emplace_back(Button({.text=std::move(text),
                .key="step-"+std::to_string(i),
                .variant=status==StepStatus::Active?ButtonVariant::Contained:ButtonVariant::Text,
                .color=color(status),
                .disabled=blocked || i==current,
                .on_click=[callback=props.on_change,before=props.on_before_change,current,i] {
                    if ((!before || before(current,i)) && callback) callback(i);
                },
                .size=ControlSize::Compact}));
            if (i+1 < props.steps.size())
                entries.emplace_back(Typography({.text=props.orientation == StepperOrientation::Vertical ? "│" : "›",
                    .key="separator-"+std::to_string(i)}));
        }
        if (props.orientation == StepperOrientation::Vertical)
            return Column({.children=std::move(entries),.key=std::move(props.key),.layout=props.layout});
        return Row({.children=std::move(entries),.key=std::move(props.key),.layout=props.layout});
    }
}; /* End of class Stepper */

} /* End of namespace terion */

#endif /* TERION_COMPONENTS_STEPPER_H */
