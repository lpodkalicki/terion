# Terion public API conventions (#139d)

Terion is a pre-release C++20 declarative TUI framework. Its API is allowed to
change without compatibility adapters. Application code should use one canonical
spelling for each concept.

## Header entry points

- `#include <terion/core.h>`: application, events, state, hooks, themes,
  clipboard, focus, geometry and core utilities.
- `#include <terion/components.h>`: the complete public component library.
- `#include <terion/render.h>`: Canvas, Painter, ScreenBuffer and low-level
  rendering tools.
- `#include <terion/terion.h>`: all three categories for convenience.
- `#include <terion/components/<name>.h>`: one component, as a direct include.

Heavy component algorithms (Chart, MarkdownView, DiffView, FilePicker) are
implemented out-of-line in `src/widgets/*.cpp`. Applications using those
components must **link** to `terion` or `terion_static` rather than rely on
header-only definitions. CMake automatically includes the new files.

## Naming

- Classes/enums: PascalCase; enum values: PascalCase.
- Fields and callbacks: `snake_case`; callback properties start with `on_`.
- Control sizes: **`ControlSize`** (`Compact`, `Medium`, `Large`).
- Text-input variants: **`InputVariant`** (`Outlined`, `Filled`).
- Do not use `ButtonSize` or `TextFieldVariant`; those aliases were deleted.
- Use aggregate `Props` with designated initializers. C++20 requires declaration
  order for *supplied* fields; changing that order is a breaking source change.

## State

- `.value` / `.checked` are the parent-controlled value of a single-value input.
- `.default_value` / `.default_checked` seed an uncontrolled input, with local
  edits retained by the reconciler while `key` is stable.
- The `FormState` behind a `Form` controls its nested inputs and supports reset.
- `key` is only used for reconciliation identity; `.id` is used for public focus
  and shortcut targeting (see #139e).

## Selection

| Component | Selected property | Change callback |
| --- | --- | --- |
| Select, RadioGroup | `value` | `on_change` |
| Table, Tabs, TreeView | `selected_key` | `on_selection_change(std::string)` |
| VirtualList | `selected_index` | `on_selection_change(int)` |
| VirtualTable | `selected_key` or `selected_index` | `on_selection_key_change(std::string)` or `on_selection_change(int)` |

`on_activate` is different from changing selection (e.g. Enter/double-click).
`selected_key` takes priority over `selected_index` in `VirtualTable` if nonempty.
Removed duplicate fields include `Table/Tabs/TreeView::value/on_change`,
`VirtualList::on_select`, and `VirtualTable::on_select/on_select_key`.

## Styling

- `ColorVariant` is semantic (resolved against the active Theme).
- `Color` is explicit RGB.
- `ColorSpec` allows either, and `TextStyleSpec` is the unified declarative
  style used in `.style_spec` where available.
- `TextStyle` is the **resolved** style for `Painter` and `ScreenBuffer`.
- Some component-specific shorthand fields (e.g. a theme color or `bold` in
  `StyledSpan`) remain because they are convenient independent defaults, not
  solely backward-compatibility aliases. Prefer `.style_spec` for rich styling.

## Static rendering helpers

- `MarkdownView::render(markdown, wrap_width)`.
- `DiffView::render(diff, mode, show_line_numbers, width, highlight_intraline)`.
- `Chart::render(...)` for basic Braille charts and `Chart::renderAdvanced(...)`
  for colored, multi-series plots.

## Contributing

1. Compile public headers independently, including the umbrella header.
2. Build GCC and Clang on C++20 with `-Wall -Wextra -pedantic -Werror`.
3. Build all examples when changing Props order.
4. Add tests for controlled/uncontrolled state, selection and direct-header use.
5. Avoid compatibility adapters and fallback overloads when the library has no
   external consumers depending on them; migrate all examples/tests together.
6. Keep sources under `src/` free of public `#include` cycles. Forward-declare
   only when the required type is complete where used.

## Component identity and focus (#139e)

`key` is exclusively a reconciliation key. `id` is the public focus/shortcut identity.
See [component-identity-focus.md](component-identity-focus.md) for contracts and migration.

## Component internals (#140)

The internal `Component::Props` is split into shared metadata and separately
owned typed payload groups. See [Component Internals 2.0](component-internals-140.md).
New components should reuse an existing behavioral group when suitable,
while retaining component-specific `Props` aggregates in public headers.
Do not add unrelated flat fields back to `Component::Props`.
