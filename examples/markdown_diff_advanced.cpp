/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <algorithm>
#include <memory>
#include <string>
#include <terion/terion.h>

using namespace terion;

namespace {

const std::string markdown = R"MD(# Terion — Text Views

Advanced Markdown preview and diff review in the terminal.

Features
========

- [x] **GFM tables** with `:---`, `:---:` and `---:` alignment
- [x] Escaped table pipes, e.g. `A\|B`
- [x] Four-backtick **fenced code blocks**
- [x] An outline with source line numbers
- [x] Underlined autolinks: <https://example.org>

| File | Status | Lines |
| :--- | :---: | ---: |
| app.cpp | **Modified** | 120 |
| notes\|draft.md | New | 42 |

## Source code

````cpp title=demo
#include <iostream>

int main() {
    // Three backticks are ordinary content in a four-backtick fence.
    std::cout << "Terion" << '\n';
}
````

### Unicode

Grapheme-safe text: zażółć gęślą jaźń · 🇵🇱 · 👨‍👩‍👧 · 👍🏽

> Click a rendered line to see its original Markdown source line.
)MD";

const std::string patch = R"DIFF(diff --git a/app.cpp b/app.cpp
index 8712abc..ff917ac 100644
--- a/app.cpp
+++ b/app.cpp
@@ -4,6 +4,6 @@
 #include <iostream>
-int main() {
-    std::cout << "Hello, world!" << '\n';
+int main(int argc, char** argv) {
+    std::cout << "Hello, Terion!" << '\n';
     return 0;
 }
@@ -20,4 +20,6 @@
 void onChange() {
-    setText("Polska 🇵🇱");
+    setText("Polska 🇵🇱 · UTF-8 👨‍👩‍👧");
+    refreshPreview();
 }
@@ -37,2 +39,3 @@
-    auto value = oldFunction();
+    auto value = newFunction();
+    updateStatus(value);
 }
)DIFF";

} /* End of namespace */

int main()
{
    Application app(std::make_unique<AnsiTerminalBackend>());
    State<DiffViewMode> mode{DiffViewMode::Unified};
    State<bool> highlight{true};
    State<bool> diff_only{false};
    State<int> active_hunk{-1};
    State<int> split_width{0};
    State<int> source_line{-1};
    State<std::string> message{"Click Markdown lines, select hunks, or drag the separator."};
    const auto hunks = DiffView::hunks(patch);
    const auto headings = MarkdownView::outline(markdown);

    app.render([&]() -> Component {
        const Size size = app.backend().size();
        const int width = std::max(1,size.width);
        const int height = std::max(1,size.height);
        const int available = std::max(1,width-2);
        const int content_height = std::max(2,height-11);
        const bool narrow = available < 48;
        const bool only_diff = diff_only.get() || narrow;
        const bool intra = highlight.get();
        const auto view_mode = mode.get();
        const int active = active_hunk.get();
        const int minimum = 18;
        const int first_width = std::clamp(split_width.get() > 0 ? split_width.get() : available/2,
            minimum,std::max(minimum,available-minimum-1));
        const int second_width = std::max(1,available-first_width-1);

        auto next_hunk = [active_hunk,hunks,active](int direction) mutable {
            if (hunks.empty()) return;
            const int count = static_cast<int>(hunks.size());
            active_hunk.set(active < 0 ? (direction > 0 ? 0 : count-1) :
                (active+direction+count) % count);
        };
        const auto markdown_callback = [source_line](std::size_t line) mutable {
            source_line.set(static_cast<int>(line)+1);
        };
        const auto diff_callback = [message](std::size_t line) mutable {
            message.set("Clicked diff source line " + std::to_string(line+1));
        };
        auto make_diff = [&](int pane_width,int pane_height) -> Component {
            return DiffView({.diff=patch,.show_line_numbers=true,
                .key="advanced-diff-view",
                .layout={.width=std::max(1,pane_width),.height=std::max(1,pane_height)},
                .show_scrollbar=true,.wrap=false,.mode=view_mode,
                .highlight_intraline=intra,.active_hunk=active,
                .on_source_line_click=diff_callback});
        };
        Component content = only_diff ?
            make_diff(available,content_height) :
            Component(ResizableSplit({.children={
                Column({.children={
                    Typography({.text="MARKDOWN   |   " + std::to_string(headings.size()) + " headings"}),
                    MarkdownView({.markdown=markdown,.key="advanced-markdown-view",
                        .layout={.width=std::max(1,first_width-2),
                            .height=std::max(1,content_height-3)},
                        .show_scrollbar=true,.on_source_line_click=markdown_callback})
                },.layout={.width=first_width,.height=content_height,.padding=1}}),
                Column({.children={
                    Typography({.text="DIFF   |   " + std::to_string(hunks.size()) + " hunks"}),
                    make_diff(std::max(1,second_width-2),std::max(1,content_height-3))
                },.layout={.width=second_width,.height=content_height,.padding=1}})
            },.direction=SplitDirection::Horizontal,.position=first_width,
                .min_first=minimum,.min_second=minimum,.key="text-view-split",
                .layout={.width=available,.height=content_height},
                .on_change=[split_width](int value) mutable {split_width.set(value);}}));

        return Column({.children={
            Typography({.text="TERION  |  #137a Markdown & Diff Enhancements",
                .variant=TypographyVariant::H2}),
            Row({.children={
                Button({.text="Unified",.on_click=[mode]() mutable {mode.set(DiffViewMode::Unified);}}),
                Button({.text="Side by side",.on_click=[mode,diff_only]() mutable {
                    mode.set(DiffViewMode::SideBySide);
                    diff_only.set(true); // The diff needs full terminal width.
                }}),
                Button({.text=intra ? "Intraline: ON" : "Intraline: OFF",
                    .on_click=[highlight,intra]() mutable {highlight.set(!intra);}}),
                Button({.text=only_diff ? "Show Markdown" : "Diff fullscreen",
                    .disabled=narrow,
                    .on_click=[diff_only,only_diff]() mutable {diff_only.set(!only_diff);}})
            },.layout={.gap=1}}),
            Row({.children={
                Button({.text="Previous hunk",.on_click=[next_hunk]() mutable {next_hunk(-1);}}),
                Button({.text="Next hunk",.on_click=[next_hunk]() mutable {next_hunk(1);}}),
                Button({.text="Copy hunk",.disabled=active<0,
                    .on_click=[active,message]() mutable {
                        if (active < 0) return;
                        Clipboard::set(DiffView::hunkText(patch,static_cast<std::size_t>(active)));
                        message.set("Hunk copied to clipboard");
                    }}),
                Typography({.text=active < 0 ? "Hunk: none" :
                    "Hunk: " + std::to_string(active+1) + "/" + std::to_string(hunks.size())})
            },.layout={.gap=1}}),
            std::move(content),
            Typography({.text="Markdown source line: " +
                (source_line.get() < 0 ? std::string("-") : std::to_string(source_line.get())) +
                "   |   " + message.get()}),
            Typography({.text="Mouse: click and scroll  |  Drag divider  |  Tab: focus  |  Esc: quit"})
        },.layout={.width=width,.height=height,.padding=1,.gap=0}});
    });
    return app.run();
}
