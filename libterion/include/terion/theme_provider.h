/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#ifndef TERION_THEME_PROVIDER_H
#define TERION_THEME_PROVIDER_H

#include <terion/context.h>

namespace terion {

namespace detail {

inline const Context<Theme> theme_context(Theme::dark());

} /* End of namespace detail */

// Reads the closest ThemeProvider. Outside providers, returns Theme::dark().
[[nodiscard]] inline Theme useTheme()
{
    return useContext(detail::theme_context);
}

[[nodiscard]] inline Component ThemeProvider(Theme theme, Component child,std::string key = {})
{
    const Theme value = theme;
    Component scope({.type = Component::Type::ThemeScope,.children = {std::move(child)},
        .theme_override = std::move(theme)});
    return ContextProvider(detail::theme_context,value,std::move(scope),std::move(key));
}

[[nodiscard]] inline Component ThemeProvider(Theme theme,std::vector<Component> children,
    std::string key = {})
{
    return ThemeProvider(std::move(theme),Fragment({.children = std::move(children)}),
        std::move(key));
}

} /* End of namespace terion */

#endif /* TERION_THEME_PROVIDER_H */
