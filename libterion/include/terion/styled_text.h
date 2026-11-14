/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#ifndef TERION_STYLED_TEXT_H
#define TERION_STYLED_TEXT_H

#include <optional>
#include <string>
#include <vector>
#include <terion/theme.h>

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
}; /* End of struct StyledSpan */

using StyledText = std::vector<StyledSpan>;

} /* End of namespace terion */

#endif /* TERION_STYLED_TEXT_H */
