/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#ifndef TERION_STYLED_TEXT_H
#define TERION_STYLED_TEXT_H

#include <optional>
#include <string>
#include <vector>
#include <terion/theme.h>
#include <terion/text_style_spec.h>

namespace terion {

struct StyledSpan
{
    std::string text;
    ColorVariant color = ColorVariant::Default;
    bool bold = false;
    bool italic = false;
    bool underline = false;
    bool strikethrough = false;
    bool dim = false;
    std::optional<ColorVariant> background = std::nullopt;
    // Explicit RGB overrides for table cells and other custom styled content.
    std::optional<Color> foreground = std::nullopt;
    std::optional<Color> background_color = std::nullopt;
    // Rich declarative style; explicitly supplied fields override simple span defaults.
    std::optional<TextStyleSpec> style_spec = std::nullopt;
}; /* End of struct StyledSpan */

using StyledText = std::vector<StyledSpan>;

} /* End of namespace terion */

#endif /* TERION_STYLED_TEXT_H */
