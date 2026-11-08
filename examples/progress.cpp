/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <algorithm>
#include <memory>
#include <string>
#include <terion/terion.h>

using namespace terion;

int main()
{
    Application app(std::make_unique<AnsiTerminalBackend>());
    State<int> percent{42};

    app.render([&] {
        const double value = percent.get()/100.0;
        return Paper({.children = {
            Typography({.text = "Progress — determinate and indeterminate",
                .variant = TypographyVariant::H2}),
            Typography({.text = "Blocks (default)"}),
            Progress({.value = value,.layout = {.width = 35},.showPercentage = true}),
            Typography({.text = "Thin line"}),
            Progress({.value = value,.layout = {.width = 35},
                .variant = ProgressVariant::Line,.showPercentage = true}),
            Typography({.text = "ASCII (also used on terminals without Unicode)"}),
            Progress({.value = value,.layout = {.width = 35},
                .variant = ProgressVariant::Ascii,.showPercentage = true}),
            Row({.children = {
                Button({.text = "-10%",.key = "minus",
                    .onClick = [&] { percent.set(std::max(0,percent.get()-10)); }}),
                Button({.text = "+10%",.key = "plus",
                    .onClick = [&] { percent.set(std::min(100,percent.get()+10)); }}),
                Button({.text = "Reset",.key = "reset",
                    .onClick = [&] { percent.set(0); }})
            },.layout = {.gap = 1}}),
            Typography({.text = "Indeterminate (automatically animated)"}),
            Progress({.layout = {.width = 35},.indeterminate = true}),
            Progress({.layout = {.width = 35},.variant = ProgressVariant::Line,
                .indeterminate = true}),
            Typography({.text = "Tab / Shift+Tab: focus   Esc: quit"})
        },.layout = {.padding = 1,.gap = 1}});
    });
    return app.run();
}
