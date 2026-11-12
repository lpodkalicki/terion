/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <memory>
#include <string_view>
#include <terion/terion.h>

using namespace terion;

namespace {

constexpr std::string_view guide = R"MD(# Terion — Markdown Showcase

A longer, scrollable document demonstrating the **MarkdownView** renderer.
Resize your terminal while reading: lines should reflow without jumping back
to the beginning of the document. Try *italic*, **bold**, ~~strikethrough~~,
`inline code`, and [a reference link](https://example.org).

---

## 1. Headings and paragraphs

### Third-level heading

This paragraph contains a deliberately long line with several words, so a narrow terminal forces wrapping and the styled spans need to remain intact. The text includes Polish characters: zażółć gęślą jaźń, and UTF-8 symbols: λ, π, ✓.

#### Fourth-level heading

MarkdownView is a **read-only** terminal component. This showcase avoids HTML,
remote images and terminal escape sequences.

## 2. Lists and indentation

- Components
  - Layout: Row, Column and Paper
    - Responsive wrapping
    - Virtualized rows
  - Text: Typography, StyledText, MarkdownView
- Input and navigation
  - Buttons and keyboard controls
  - Mouse wheel and scrollbar
- A long item in a nested list that should wrap onto additional visual rows while maintaining the indentation of its continuation rather than restarting at the left edge.

1. Start the application.
2. Resize the terminal.
3. Scroll through the document.
4. Inspect the formatted tables and code blocks.

### Checklist

- [x] Styled headings and inline formatting
- [x] Nested lists and hanging indentation
- [x] Markdown tables
- [x] Syntax highlighting in code fences
- [ ] Full CommonMark/GFM compliance

## 3. Quotes

> Make the common case readable.
> Keep the terminal interface predictable.

## 4. Tables

| Component | Feature | Status |
| :--- | :--- | ---: |
| MarkdownView | Styled paragraphs | Ready |
| MarkdownView | Nested lists | Ready |
| MarkdownView | Tables | Ready |
| MarkdownView | Width-aware reflow | Ready |
| DiffView | Unified diff display | Ready |

### Larger table

| Name | Language | Description | Priority |
| :--- | :--- | :--- | ---: |
| Parser | C++20 | Terminal-oriented Markdown parser | 1 |
| Renderer | C++20 | Styled spans for on-screen text | 2 |
| Layout | C++20 | Word wrap and viewport layout | 3 |
| Tests | C++20 | Headless rendering checks | 4 |

## 5. Code fences

### C++

```cpp
#include <string>
#include <vector>

int main()
{
    const std::string greeting = "Hello, Terion!";
    std::vector<int> values{1, 2, 3, 5, 8};
    // This comment is highlighted separately.
    return values.empty() ? 1 : 0;
}
```

### Python

```python
def greet(name):
    # Strings, keywords and numbers have different styles.
    message = "Hello, " + name
    for number in range(3):
        print(message, number)

greet("Terion")
```

### Bash

```bash
#!/usr/bin/env bash
set -euo pipefail
for item in one two three; do
    echo "item: $item"
done
```

### JSON

```json
{
  "component": "MarkdownView",
  "enabled": true,
  "maxLines": 120,
  "languages": ["cpp", "python", "bash", "json"]
}
```

## 6. End of document

You have reached the end of the main demonstration. Use the buttons above
to switch to the documentation or release-notes sample.
)MD";

constexpr std::string_view reference = R"MD(# MarkdownView — Component Guide

## Quick start

```cpp
MarkdownView({
    .markdown = markdown,
    .key = "markdown-preview",
    .layout = {.height = 22},
    .wrap = true,
    .wrap_width = 0
});
```

## Configuration

| Property | Type | Purpose |
| :--- | :--- | :--- |
| `markdown` | string | Source document |
| `key` | string | Stable component identity |
| `layout` | LayoutStyle | Size constraints |
| `wrap` | bool | Enable line wrapping |
| `wrap_width` | int | 0 means available width |
| `show_scrollbar` | bool | Show vertical scrollbar |
| `disabled` | bool | Disable interactions |

## Tips

- Leave `wrap_width` at `0` to follow the available viewport width.
- Set `.layout.height` to create a scrollable document region.
- Keep a stable `.key` when the parent component re-renders.
- Use a real Markdown parser when you need complete CommonMark compliance.

## Interaction

1. Click the view to focus it.
2. Scroll using the wheel or the scrollbar.
3. Use keyboard navigation supported by `VirtualList`.
4. Resize the terminal to see the content reflow.

> This is a terminal-oriented subset of Markdown, not an HTML browser.
)MD";

constexpr std::string_view changes = R"MD(# Release Notes — #132b

## Added

- Tables with Unicode separators and styled headers.
- Nested unordered and ordered lists.
- Hanging indentation for wrapped list items.
- Responsive word wrapping on terminal resize.
- Approximate scroll-position preservation after reflow.

## Example changes

| Patch | Area | Highlight |
| :--- | :--- | :--- |
| #132 | Text views | MarkdownView and DiffView |
| #132a | Rich text | Wrapping and syntax highlighting |
| #132b | Markdown | Tables, nested lists and reflow |

## Testing checklist

- [x] Build with C++20 and `-Werror`
- [ ] Check narrow terminal: 50 columns
- [ ] Check wide terminal: 120 columns
- [ ] Scroll to the middle and resize
- [ ] Verify the table remains readable

## Notes

Long table cells currently use basic width handling. Complex tables, embedded
HTML and complete GFM support are deliberately outside this demonstration.
)MD";

} // namespace

int main()
{
    Application app(std::make_unique<AnsiTerminalBackend>());
    State<int> page{0};

    app.render([&] {
        const int selected = page.get();
        const std::string_view markdown = selected == 0 ? guide :
            selected == 1 ? reference : changes;

        return Paper({.children = {
            Typography({.text = "MarkdownView — Advanced Markdown (#132b)",
                .variant = TypographyVariant::H2}),
            Typography({.text = "Click to focus | Wheel / arrows / PgUp / PgDn | Resize terminal to test reflow"}),
            Row({.children = {
                Button({.text = selected == 0 ? "[Showcase]" : "Showcase",
                    .key = "showcase",.on_click = [&] { page.set(0); }}),
                Button({.text = selected == 1 ? "[API guide]" : "API guide",
                    .key = "guide",.on_click = [&] { page.set(1); }}),
                Button({.text = selected == 2 ? "[Release notes]" : "Release notes",
                    .key = "changes",.on_click = [&] { page.set(2); }})
            },.layout = {.gap = 1}}),
            MarkdownView({.markdown = std::string(markdown),
                .key = "markdown-showcase-"+std::to_string(selected),
                .layout = {.height = 24},.show_scrollbar = true,
                .wrap = true,.wrap_width = 0}),
            Typography({.text = "Rendered using MarkdownView + VirtualList + StyledText"})
        },.layout = {.padding = 1,.gap = 1}});
    });

    return app.run();
}
