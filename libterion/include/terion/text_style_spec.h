/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */
#ifndef TERION_TEXT_STYLE_SPEC_H
#define TERION_TEXT_STYLE_SPEC_H

#include <optional>
#include <variant>
#include <terion/theme.h>

namespace terion {

// A color may be tied to the current theme or be an explicit RGB color.
// Unlike render::TextStyle, this specification is resolved at PAINT time.
struct ColorSpec
{
    std::variant<ColorVariant,Color> value = ColorVariant::Default;

    constexpr ColorSpec() = default;
    constexpr ColorSpec(ColorVariant semantic) : value(semantic) {}
    constexpr ColorSpec(Color rgb) : value(rgb) {}

    [[nodiscard]] constexpr Color resolveForeground(const Palette& palette) const noexcept
    {
        if (const auto* rgb = std::get_if<Color>(&value)) return *rgb;
        return palette.resolve(std::get<ColorVariant>(value));
    }
    [[nodiscard]] constexpr Color resolveBackground(const Palette& palette) const noexcept
    {
        if (const auto* rgb = std::get_if<Color>(&value)) return *rgb;
        const auto variant = std::get<ColorVariant>(value);
        return variant == ColorVariant::Default ? palette.background : palette.resolve(variant);
    }
    bool operator==(const ColorSpec&) const = default;
}; /* End of struct ColorSpec */

// Declarative/semantic style. Optional flags permit explicit false overrides
// without resetting styles inherited from a component, row or selection.
// TextStyle remains the fully resolved RGB style used by Painter/ScreenBuffer.
struct TextStyleSpec
{
    std::optional<ColorSpec> foreground = std::nullopt;
    std::optional<ColorSpec> background = std::nullopt;
    std::optional<bool> bold = std::nullopt;
    std::optional<bool> italic = std::nullopt;
    std::optional<bool> underline = std::nullopt;
    std::optional<bool> strikethrough = std::nullopt;
    std::optional<bool> dim = std::nullopt;

    [[nodiscard]] constexpr TextStyle resolve(const Palette& palette,TextStyle base = {}) const noexcept
    {
        if (foreground) base.foreground = foreground->resolveForeground(palette);
        if (background) base.background = background->resolveBackground(palette);
        if (bold) base.bold = *bold;
        if (italic) base.italic = *italic;
        if (underline) base.underline = *underline;
        if (strikethrough) base.strikethrough = *strikethrough;
        if (dim) base.dim = *dim;
        return base;
    }
    bool operator==(const TextStyleSpec&) const = default;
}; /* End of struct TextStyleSpec */

} /* End of namespace terion */
#endif /* TERION_TEXT_STYLE_SPEC_H */
