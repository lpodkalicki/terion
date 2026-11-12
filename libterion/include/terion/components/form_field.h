/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#ifndef TERION_COMPONENTS_FORM_FIELD_H
#define TERION_COMPONENTS_FORM_FIELD_H

#include <stdexcept>
#include <utility>
#include <terion/component.h>

namespace terion {

// Form resolves this marker within its own render scope; fields do not own state.
class FormField : public Component
{
public:
    struct Props
    {
        std::string name = {};
        std::string label = {};
        bool required = false;
        Component control = Component::text("");
        std::string key = {};
        LayoutStyle layout = {};
    }; /* End of struct Props */

    explicit FormField(Props props) : Component(create(std::move(props))) {}

private:
    static Component create(Props props)
    {
        if (props.name.empty()) throw std::invalid_argument("FormField requires a nonempty name");
        return Component({.type = Type::FormFieldMarker,.key = std::move(props.key),
            .children = {std::move(props.control)},.layout = props.layout,
            .form_field_name = std::move(props.name),
            .form_field_label = std::move(props.label),
            .form_field_required = props.required});
    }
}; /* End of class FormField */

} /* End of namespace terion */

#endif /* TERION_COMPONENTS_FORM_FIELD_H */
