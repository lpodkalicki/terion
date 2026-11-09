/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#ifndef TERION_COMPONENTS_ERROR_BOUNDARY_H
#define TERION_COMPONENTS_ERROR_BOUNDARY_H

#include <terion/component.h>
#include <functional>
#include <stdexcept>

namespace terion {

class ErrorBoundary : public Component
{
public:
    struct Props
    {
        Component child = Component::column({});
        std::function<Component(const std::string&)> fallback = {};
        std::string key = {};
    };

    explicit ErrorBoundary(Props props) :
        Component({.type = Type::ErrorBoundary,.key = std::move(props.key),
            .children = {std::move(props.child)},.error_fallback = std::move(props.fallback)})
    {
        if (!this->props().error_fallback)
            throw std::invalid_argument("ErrorBoundary requires a fallback");
    }
};

} /* End of namespace terion */

#endif /* TERION_COMPONENTS_ERROR_BOUNDARY_H */
