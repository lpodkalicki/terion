/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#ifndef TERION_THEME_H
#define TERION_THEME_H

#include <string_view>
#include <terion/render/color.h>

namespace terion {

enum class ColorVariant { Default, Primary, Success, Warning, Error, Info };

struct Palette
{
    Color primary = Color::rgb(90,160,255);
    Color on_primary = Color::rgb(0,0,0);
    Color foreground = Color::rgb(235,235,240);
    Color background = Color::rgb(20,22,28);
    Color surface = Color::rgb(38,42,52);
    Color muted = Color::rgb(105,110,120);
    Color success = Color::rgb(88,206,132);
    Color on_success = Color::rgb(12,38,21);
    Color warning = Color::rgb(255,194,81);
    Color on_warning = Color::rgb(53,32,6);
    Color error = Color::rgb(255,104,112);
    Color on_error = Color::rgb(55,10,17);
    Color info = Color::rgb(105,191,255);
    Color on_info = Color::rgb(12,29,49);
    [[nodiscard]] constexpr Color resolve(const ColorVariant variant) const noexcept
    {
        switch (variant)
        {
            case ColorVariant::Primary: return primary;
            case ColorVariant::Success: return success;
            case ColorVariant::Warning: return warning;
            case ColorVariant::Error: return error;
            case ColorVariant::Info: return info;
            default: return foreground;
        }
    }
    [[nodiscard]] constexpr Color onColor(const ColorVariant variant) const noexcept
    {
        switch (variant)
        {
            case ColorVariant::Primary: return on_primary;
            case ColorVariant::Success: return on_success;
            case ColorVariant::Warning: return on_warning;
            case ColorVariant::Error: return on_error;
            case ColorVariant::Info: return on_info;
            default: return background;
        }
    }
    bool operator==(const Palette&) const = default;
}; /* End of struct Palette */

struct Spacing
{
    int unit = 1;
    constexpr int operator()(int multiplier) const { return unit * multiplier; }
    bool operator==(const Spacing&) const = default;
}; /* End of struct Spacing */

struct ScrollbarStyle
{
    // Single-cell Unicode glyphs keep geometry and mouse hit testing unchanged.
    std::string_view track = "│";
    std::string_view thumb = "┃";
    Color track_color = Color::rgb(105,110,120);
    Color thumb_color = Color::rgb(235,235,240);
    Color hover_color = Color::rgb(90,160,255);
    Color active_color = Color::rgb(90,160,255);
    bool operator==(const ScrollbarStyle&) const = default;
}; /* End of struct ScrollbarStyle */

struct Theme;
[[nodiscard]] constexpr Theme withScrollbarColors(Theme theme) noexcept;

struct Theme
{
    Palette palette = {};
    Spacing spacing = {};
    ScrollbarStyle scrollbar = {};
    bool operator==(const Theme&) const = default;

    static constexpr Theme dark() { return {}; }
    static constexpr Theme light()
    {
        return withScrollbarColors({.palette = {.primary = Color::rgb(35,96,205),
                            .on_primary = Color::rgb(255,255,255),
                            .foreground = Color::rgb(30,35,42),
                            .background = Color::rgb(248,249,251),
                            .surface = Color::rgb(228,231,238),
                            .muted = Color::rgb(115,120,130),
                            .success = Color::rgb(25,124,71),.on_success = Color::rgb(255,255,255),
                            .warning = Color::rgb(165,95,8),.on_warning = Color::rgb(255,255,255),
                            .error = Color::rgb(188,45,55),.on_error = Color::rgb(255,255,255),
                            .info = Color::rgb(26,111,166),.on_info = Color::rgb(255,255,255)}});
    }
    static constexpr Theme ocean() { return withScrollbarColors({.palette = {
        .primary = Color::rgb(51,194,224),.on_primary = Color::rgb(6,32,46),
        .foreground = Color::rgb(225,244,248),.background = Color::rgb(8,28,42),
        .surface = Color::rgb(18,53,68),.muted = Color::rgb(102,149,160)}}); }
    static constexpr Theme forest() { return withScrollbarColors({.palette = {
        .primary = Color::rgb(137,211,107),.on_primary = Color::rgb(19,42,26),
        .foreground = Color::rgb(228,239,222),.background = Color::rgb(21,34,26),
        .surface = Color::rgb(40,57,42),.muted = Color::rgb(128,154,126)}}); }
    static constexpr Theme sunset() { return withScrollbarColors({.palette = {
        .primary = Color::rgb(255,155,88),.on_primary = Color::rgb(55,28,31),
        .foreground = Color::rgb(255,235,223),.background = Color::rgb(43,25,38),
        .surface = Color::rgb(72,42,55),.muted = Color::rgb(171,127,132)}}); }
    static constexpr Theme violet() { return withScrollbarColors({.palette = {
        .primary = Color::rgb(194,154,255),.on_primary = Color::rgb(32,21,51),
        .foreground = Color::rgb(239,230,255),.background = Color::rgb(25,20,43),
        .surface = Color::rgb(46,37,68),.muted = Color::rgb(145,127,171)}}); }
    static constexpr Theme nord() { return withScrollbarColors({.palette = {
        .primary = Color::rgb(136,192,208),.on_primary = Color::rgb(46,52,64),
        .foreground = Color::rgb(236,239,244),.background = Color::rgb(46,52,64),
        .surface = Color::rgb(59,66,82),.muted = Color::rgb(129,161,193)}}); }
    static constexpr Theme dracula() { return withScrollbarColors({.palette = {
        .primary = Color::rgb(189,147,249),.on_primary = Color::rgb(40,42,54),
        .foreground = Color::rgb(248,248,242),.background = Color::rgb(40,42,54),
        .surface = Color::rgb(68,71,90),.muted = Color::rgb(139,147,168)}}); }
    static constexpr Theme solarized() { return withScrollbarColors({.palette = {
        .primary = Color::rgb(38,139,210),.on_primary = Color::rgb(253,246,227),
        .foreground = Color::rgb(131,148,150),.background = Color::rgb(0,43,54),
        .surface = Color::rgb(7,54,66),.muted = Color::rgb(88,110,117)}}); }
}; /* End of struct Theme */

[[nodiscard]] constexpr Theme withScrollbarColors(Theme theme) noexcept
{
    theme.scrollbar.track_color = theme.palette.muted;
    theme.scrollbar.thumb_color = theme.palette.foreground;
    theme.scrollbar.hover_color = theme.palette.primary;
    theme.scrollbar.active_color = theme.palette.primary;
    return theme;
}

} /* End of namespace terion */

#endif /* TERION_THEME_H */
