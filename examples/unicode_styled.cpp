/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <memory>
#include <string>
#include <terion/terion.h>

using namespace terion;

int main()
{
    Application app(std::make_unique<AnsiTerminalBackend>());
    State<std::string> input{"Zażółć gęślą jaźń · e\u0301 · 👨‍👩‍👧 · 🇵🇱 · 👍🏽"};
    const StyledText samples = {
        {.text = "Bold  ",.color = ColorVariant::Primary,.bold = true},
        {.text = "Italic  ",.color = ColorVariant::Info,.italic = true},
        {.text = "Underline  ",.color = ColorVariant::Success,.underline = true},
        {.text = "Strike  ",.color = ColorVariant::Warning,.strikethrough = true},
        {.text = "Dim",.dim = true},
        {.text = "  Background",.bold = true,.background = ColorVariant::Info}
    };
    app.render([&] {
        const Size window = app.backend().size();
        const int width = std::max(20,window.width-4);
        return Column({.children = {
            Typography({.text = "#135c  |  StyledText & Unicode",.variant = TypographyVariant::H2}),
            Typography({.text = "SGR formatting (ANSI terminal support varies):"}),
            VirtualList({.item_count = 1,.item_styled = [samples](int) { return samples; },
                .layout = {.width = width,.height = 2},.show_scrollbar = false}),
            Typography({.text = "Graphemes: e\u0301  🇵🇱  👨‍👩‍👧  👍🏽  ❤️  1️⃣"}),
            Typography({.text = "Edit and select complete graphemes (left/right, backspace, mouse):"}),
            TextArea({.value = input.get(),.key = "unicode-editor",.soft_wrap = true,
                .layout = {.width = width,.height = 5},
                .on_change = [&](std::string value) { input.set(std::move(value)); }}),
            Typography({.text = "Markdown: italic, strike and underlined links:"}),
            MarkdownView({.markdown = "*Italic* · ~~Strike~~ · [Link](https://example.org)",
                .layout = {.width = width,.height = 3},.show_scrollbar = false}),
            Typography({.text = "Tab: focus  |  Mouse drag: selection  |  Esc: quit"})
        },.layout = {.width = window.width,.height = window.height,.padding = 1,.gap = 1}});
    });
    return app.run();
}
