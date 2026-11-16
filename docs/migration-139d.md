# Migration to the canonical Terion API (#139d)

This is an **intentional breaking change** for the pre-release library.
There are no deprecated aliases or compatibility adapters for these APIs.

| Removed API | Canonical replacement |
| --- | --- |
| `ButtonSize` | `ControlSize` |
| `TextFieldVariant` | `InputVariant` |
| `Table::Props::value` | `selected_key` |
| `Table::Props::on_change` | `on_selection_change` |
| `Tabs::Props::value` | `selected_key` |
| `Tabs::Props::on_change` | `on_selection_change` |
| `TreeView::Props::value` | `selected_key` |
| `TreeView::Props::on_change` | `on_selection_change` |
| `VirtualList::Props::on_select` | `on_selection_change` |
| `VirtualTable::Props::on_select` | `on_selection_change` |
| `VirtualTable::Props::on_select_key` | `on_selection_key_change` |
| `DiffView::renderEnhanced(...)` | `DiffView::render(diff, mode, numbers, width, highlight)` |
| Former 3-argument `DiffView::render(diff, numbers, wrap)` | `DiffView::render(diff, DiffViewMode::Unified, numbers, wrap, false)` |
| `Component::Props::button_size` | `control_size` |
| `Component::Props::text_field_variant` | `input_variant` |

**C++20 designated initializers:** fix the *order* of fields if you migrate an
old constructor; for example, `Table::Props::selected_key` precedes `key`,
while `VirtualTable::Props::on_selection_change` precedes `on_sort`.

## Headers

```cpp
#include <terion/core.h>
#include <terion/components.h>
#include <terion/render.h>
// Or simply #include <terion/terion.h>
```

Single-component headers remain available. Large components (`Chart`,
`MarkdownView`, `DiffView`, `FilePicker`) are no longer header-only: link to the
library rather than compiling users without its implementation objects.

## CMake consumer

```cmake
find_package(Terion CONFIG REQUIRED)
target_link_libraries(my_app PRIVATE Terion::terion)
# Or Terion::terion_static
```

The project exports `TerionConfig.cmake`, version metadata, and imported targets
under the standard `lib/cmake/Terion` layout. `Threads` is loaded transitively.

## What remains unchanged

`Select` and `RadioGroup` continue to use `value` and `on_change` because they
represent a single scalar choice. Nonredundant styling shorthands and the
`Chart::render` simple Braille overload remain. Synchronous `FilePicker`
listing remains a supported execution mode for simple/local applications;
async listing is optional, not merely a deprecated alias.
