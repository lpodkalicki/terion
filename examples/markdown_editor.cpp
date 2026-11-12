/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <algorithm>
#include <memory>
#include <string>
#include <utility>
#include <terion/terion.h>

using namespace terion;

namespace {

constexpr const char* initial_markdown = R"MD(# Terion Markdown Editor

Edit the **Markdown source** on the left. The preview on the right updates as you type.

## Formatting

A paragraph with *emphasis*, **strong text**, ~~strikethrough~~ and `inline code`.

> A terminal-first editor with a live preview.

---

## Nested lists

- Rendering
  - Styled text and **UTF-8**
  - Responsive word wrapping
    - Resize the terminal or drag the divider
- Input
  - Edit, select and paste text
  - Scroll each pane independently

1. Change this text.
2. Drag the separator.
3. Try a smaller terminal window.

## Checklist

- [x] Live Markdown preview
- [x] Resizable split panes
- [ ] Write your own document

## Components

| Component | Purpose | Status |
| :--- | :--- | ---: |
| TextArea | Edit source | Ready |
| MarkdownView | Preview | Ready |
| ResizableSplit | Resize panes | Ready |

## C++ example

```cpp
#include <iostream>

int main()
{
    // Try editing this code block!
    std::cout << "Hello, Terion!" << '\n';
}
```

## JSON example

```json
{"name": "Terion", "preview": true, "version": 132}
```

Polskie znaki: zażółć gęślą jaźń. Unicode: λ π ✓

Keep typing below to try more Markdown.
)MD";

int countLines(const std::string& text)
{
    return static_cast<int>(std::count(text.begin(),text.end(),'\n'))+1;
}

} /* End of namespace */

int main()
{
    Application app(std::make_unique<AnsiTerminalBackend>());
    State<std::string> source{std::string(initial_markdown)};
    State<int> left_width{0};

    app.render([&] {
        const Size screen = app.backend().size();
        const int width = std::max(0,screen.width);
        const int height = std::max(0,screen.height);
        const int pane_height = std::max(1,height-4);
        const int available_width = std::max(1,width-2);
        const int min_pane_width = 12;
        const int requested_width = left_width.get() > 0 ? left_width.get() : available_width/2;
        const int first_width = std::clamp(requested_width,min_pane_width,
            std::max(min_pane_width,available_width-min_pane_width-1));
        const int second_width = std::max(1,available_width-first_width-1);
        const std::string markdown = source.get();

        return Column({.children = {
            Row({.children = {
                Typography({.text = "Markdown Editor",.variant = TypographyVariant::H2}),
                Typography({.text = "  LIVE PREVIEW  |  #132b"})
            },.layout = {.height = 1,.gap = 1}}),
            ResizableSplit({.children = {
                Column({.children = {
                    Typography({.text = "SOURCE  ·  Markdown"}),
                    TextArea({.value = markdown,.placeholder = "Type Markdown here...",
                        .key = "markdown-source",
                        .min_rows = 1,
                        .layout = {.width = std::max(1,first_width-2),
                            .height = std::max(1,pane_height-3)},
                        .on_change = [&](std::string value) { source.set(std::move(value)); }})
                },.layout = {.width = first_width,.height = pane_height,.padding = 1,.gap = 0}}),
                Column({.children = {
                    Typography({.text = "PREVIEW  ·  Rendered Markdown"}),
                    MarkdownView({.markdown = markdown,.key = "markdown-preview",
                        .layout = {.width = std::max(1,second_width-2),
                            .height = std::max(1,pane_height-3)},
                        .show_scrollbar = true,.wrap = true,.wrap_width = 0})
                },.layout = {.width = second_width,.height = pane_height,.padding = 1,.gap = 0}})
            },.direction = SplitDirection::Horizontal,.position = first_width,
                .min_first = min_pane_width,.min_second = min_pane_width,
                .key = "markdown-editor-split",
                .layout = {.width = available_width,.height = pane_height},
                .on_change = [&](int value) { left_width.set(value); }}),
            Typography({.text = "Drag divider to resize  |  Tab: change focus  |  " +
                std::to_string(countLines(markdown)) + " lines  ·  " +
                std::to_string(markdown.size()) + " bytes"})
        },.layout = {.width = width,.height = height,.padding = 1,.gap = 0}});
    });

    return app.run();
}
