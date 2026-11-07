/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#ifndef TERION_COMPONENT_H
#define TERION_COMPONENT_H

#include <string>
#include <utility>
#include <vector>

namespace terion {

class Component
{
public:
    enum class Type
    {
        Text,
        Column
    }; /* End of enum Type */

    struct Props
    {
        Type type = Type::Column;
        std::string key;
        std::string text;
        std::vector<Component> children;
    }; /* End of struct Props */

    explicit Component(Props props) : props_(std::move(props)) {}

    static Component text(std::string value, std::string key = {});
    static Component column(std::vector<Component> children, std::string key = {});

    [[nodiscard]] const Props& props() const noexcept { return props_; }

private:
    Props props_;
}; /* End of class Component */

} /* End of namespace terion */

#endif /* TERION_COMPONENT_H */
