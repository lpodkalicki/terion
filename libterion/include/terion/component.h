/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#ifndef TERION_COMPONENT_H
#define TERION_COMPONENT_H

#include <string>
#include <functional>
#include <optional>
#include <utility>
#include <vector>
#include <terion/layout.h>
#include <terion/render/color.h>

namespace terion {

enum class ButtonVariant
{
    Text,
    Outlined,
    Contained
}; /* End of enum ButtonVariant */

class Component
{
public:
    enum class Type
    {
        Text,
        Column,
        Row,
        Box,
        Button,
        TextField,
        Divider,
        Progress
    }; /* End of enum Type */

    struct Props
    {
        Type type = Type::Column;
        std::string key = {};
        std::string text = {};
        std::vector<Component> children = {};
        LayoutStyle layout = {};
        TextStyle text_style = {};
        std::function<void()> on_click = {};
        std::function<void(std::string)> on_change = {};
        std::string placeholder = {};
        bool disabled = false;
        std::optional<BorderStyle> border = std::nullopt;
        std::optional<Color> background = std::nullopt;
        ButtonVariant button_variant = ButtonVariant::Outlined;
        double value = 0.0;
    }; /* End of struct Props */

    explicit Component(Props props) : props_(std::move(props)) {}

    static Component text(std::string value, std::string key = {});
    static Component column(std::vector<Component> children, std::string key = {}, LayoutStyle layout = {});
    static Component row(std::vector<Component> children, std::string key = {}, LayoutStyle layout = {});
    static Component box(std::vector<Component> children, LayoutStyle layout = {}, std::string key = {});

    [[nodiscard]] const Props& props() const noexcept { return props_; }

private:
    Props props_;
}; /* End of class Component */

} /* End of namespace terion */

#endif /* TERION_COMPONENT_H */
