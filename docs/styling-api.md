# Styling API (#139c)

Terion separates **declarative styles** (semantic colors resolved for the active theme)
from **raster styles** (fully resolved RGB colors used by `Painter` and `ScreenBuffer`).
This avoids changing existing low-level `TextStyle` and supports C++20 aggregate initializers in public `Props`.

## Types

- `Color` is a literal 24-bit RGB color, e.g. `Color::rgb(24, 48, 74)`.
- `ColorVariant` is a semantic color defined by the current theme (`Primary`,
  `Success`, `Warning`, `Error`, `Info`, `Default`).
- `ColorSpec` accepts **either** `Color` or `ColorVariant`, resolved against a
  `Palette` at paint time. `Default` resolves to the palette foreground for
  text, or the palette background for backgrounds.
- `TextStyleSpec` is a partial text style. All fields are optional. It supports
  `foreground`, `background`, `bold`, `italic`, `underline`, `strikethrough`,
  and `dim`. Explicit `false` removes an inherited decoration.
- `TextStyle` is the *existing*, fully resolved RGB rendering type. It remains
  supported in the `style` fields of components and in `Painter`.

## Usage

```cpp
Typography({
    .text = "Warning",
    .style_spec = TextStyleSpec{
        .foreground = ColorVariant::Warning,
        .bold = true
    }
});

StyledText spans{
    {.text = "error", .style_spec = TextStyleSpec{
        .foreground = Color::rgb(255, 90, 90),
        .background = ColorVariant::Default,
        .underline = true
    }}
};

TableCellFormat cell{
    .text = "ONLINE",
    .style_spec = TextStyleSpec{
        .foreground = ColorVariant::Success,
        .background = Color::rgb(20, 35, 50),
        .bold = true
    }
};
```

`style_spec` is available in `Typography`, `Button`, `Divider`, `Progress`,
`StyledSpan` and `TableCellFormat` (therefore `Table` and `VirtualTable`),
as well as `TextField`, `TextArea`, `NumberField` and `PasswordField`.
`VirtualList`, `LogView`, `MarkdownView`, and `DiffView` carry `StyledSpan` styles
without requiring an additional component API.

## Style resolution and precedence

1. Components first compute their normal text style from their state,
   inherited background, active theme, base `style` / `color` / RGB fields.
2. `style_spec` overrides **only the explicitly set** fields. For example,
   `.italic = true` does not reset a cell's bold selection style.
3. A disabled table row or virtual list ignores the custom style to preserve
   the disabled indication. Button styling also preserves its disabled state.
4. Colors are resolved using the palette active during paint, so changing
   `ThemeProvider` updates semantic colors even with retained components.

Base `StyledSpan.color`, `.background`, `.foreground`, `.background_color`,
`TableCellFormat.foreground`, `.background` and `.bold` remain valid as concise style fields. For complete declarations, prefer
`style_spec`.

For rendering APIs that need exact RGB values, use the existing `TextStyle`.
Do not resolve semantic colors in component constructors: the theme may change.

See `examples/styling_api.cpp` for an interactive dark/light theme preview.
