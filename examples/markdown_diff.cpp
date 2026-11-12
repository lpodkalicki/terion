/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <memory>
#include <terion/terion.h>

using namespace terion;

int main()
{
    Application app(std::make_unique<AnsiTerminalBackend>());
    constexpr auto markdown = R"(# MarkdownView

Read-only **Markdown** with `inline code`, [links](https://example.org) and lists.

## Features
- Styled headings and **bold** text
- [x] Virtual scrolling
- [ ] Inline images (not supported)
> Quoted text looks different.

```cpp
int main() { return 0; }
```
)";
    constexpr auto diff = R"(diff --git a/example.cpp b/example.cpp
--- a/example.cpp
+++ b/example.cpp
@@ -1,3 +1,4 @@
 int main() {
-  return 1;
+  // New implementation
+  return 0;
 }
)";
    app.render([&] {
        return Paper({.children = {
            Typography({.text = "#132 — MarkdownView + DiffView",.variant = TypographyVariant::H2}),
            Row({.children = {
                MarkdownView({.markdown = markdown,.layout = {.width = 54,.height = 18}}),
                DiffView({.diff = diff,.layout = {.width = 64,.height = 18}})
            },.layout = {.gap = 2}})
        },.layout = {.padding = 1,.gap = 1}});
    });
    return app.run();
}
