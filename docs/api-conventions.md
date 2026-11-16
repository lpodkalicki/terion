# Terion public API conventions

**Status:** adopted in #139a. This document describes the API shipped in the
current source tree; proposals for later releases are explicitly identified.

Terion is a C++20 declarative TUI framework. Keep component construction based
on aggregates and designated initializers:

```cpp
TextField({
    .value = hostname,
    .on_change = setHostname,
    .variant = InputVariant::Outlined,
    .size = ControlSize::Medium
});
```

## Names, casing and header layout

| Item | Convention | Example |
| --- | --- | --- |
| Public classes and enum types | `PascalCase` | `TextField`, `ControlSize` |
| Public enum values | `PascalCase` | `ControlSize::Compact` |
| Properties, methods and free functions | `snake_case` / existing `camelCase` methods retained | `.read_only`, `Theme::controlMetrics()` |
| Callback properties | `on_` + event or action | `.on_change`, `.on_click`, `.on_activate` |
| Public header paths | `<terion/components/name.h>` | `<terion/components/text_field.h>` |
| Include guards | `TERION_..._H` | `TERION_COMPONENTS_TEXT_FIELD_H` |

Existing camelCase methods (`controlMetrics`, `focusColor`) stay unchanged in
#139a. Renaming those methods would add noise without improving interoperability.

A component exposes a single `Props` aggregate passed by value to its explicit
constructor. All optional properties must have useful defaults. **Never reorder
existing fields:** C++20 designated initializers require declaration order.
Append new fields to the end of a public `Props` aggregate when maintaining source
compatibility. If a new callback changes behavior, test its default-initialized
behavior as well as the explicitly provided callback.

## Sizes, variants and density

Use **`ControlSize`** for the `.size` property of buttons, fields, selects,
autocomplete and dropdown menus. Its values are `Compact`, `Medium`, `Large`.
`ButtonSize` is retained as an exact type alias relationship: both names refer
to the *same* enum type; there are no conversions or extra overloads. `Compact`
is deliberately preserved rather than renamed to `Small`.

Use **`InputVariant`** for outlined/filled text, password and number fields.
`TextFieldVariant` remains a supported alias to the same enum type.

```cpp
Button({.text = "Save", .size = ControlSize::Compact});
NumberField({.value = 42.0, .variant = InputVariant::Filled,
             .size = ControlSize::Medium});
// Unchanged old code also compiles:
Button({.text = "Save", .size = ButtonSize::Compact});
TextField({.variant = TextFieldVariant::Outlined});
```

`ComponentDensity` configures the *theme-wide* spacing density and does not
replace `.size`: `Compact`, `Comfortable`, `Spacious` respectively adjust the
metrics. `Theme::controlMetrics()` accepts either spelling of the size type.

## Values, selection and callbacks

These are the **current**, not aspirational, rules:

- Input-like widgets (`TextField`, `TextArea`, `Select`, `Tabs`, `Checkbox`,
  `Switch`, `RadioGroup`, `Slider`) use `value`/`checked` and `on_change` where
  supported. The precise controlled/uncontrolled behavior varies by widget;
  #139b will specify and unify it.
- Button-like actions use `on_click`, dialogs use action-specific callbacks,
  and virtual lists expose `on_select` (selection) and `on_activate` (activation).
- Collection widgets are **not yet fully uniform**: `Table` uses `value` and
  `on_change` for its selected key; `VirtualTable` exposes `selected_index`,
  `selected_key`, `on_select`, `on_select_key`. Do not assume these names are
  interchangeable. Their future normalization belongs to #139b.
- Use `on_event_capture` and `on_event` for low-level event propagation;
  higher-level callbacks should normally be sufficient for applications.

A callback with `on_change` reports the **new value** rather than the old value.
A callback must not be invoked merely because the component was rendered.
Avoid two callbacks with competing ownership of the same state.

## Identity, focus and layout

`key` is the currently supported reconciliation identity, and in existing code
it also participates in focus lookup. **Do not introduce `.id` in application
code yet**; separating identity from focus targets is planned for #139e.
Maintain stable unique keys among siblings where state retention matters.

Use `LayoutStyle` for geometry; retain literal `.gap` and `.padding` semantics.
`SpacingToken` and the `gap_token`/`padding_token` properties from #138 are
optional additions, not implicit replacements for literal values.

Public components must not depend on backend-specific ANSI escape sequences.
For components that display Unicode, prefer shared grapheme/width utilities
rather than counting bytes; terminal-specific emoji fallback remains separate.

## Compatibility promises for patches #139a–#139e

1. Keep **source compatibility** with documented existing component initializers.
2. Do not reorder, delete or silently reinterpret existing public `Props` fields.
3. Prefer aliases and adapters before removing old spellings; postpone removals
   to a separately announced breaking release.
4. Keep the default rendering and input behavior unchanged when introducing an
   optional property.
5. Add headless behavior tests *and* compile-time public-API tests for changes.

#139a introduces only additive names / source-level spelling changes.
It does not refactor `Component::Props`, state ownership or event propagation.

## #139a migration cheatsheet

| Existing | Preferred going forward | Required to migrate? |
| --- | --- | --- |
| `ButtonSize::Compact` | `ControlSize::Compact` | No |
| `ButtonSize::Medium` | `ControlSize::Medium` | No |
| `TextFieldVariant::Outlined` | `InputVariant::Outlined` | No |
| `.size = ButtonSize::Large` | `.size = ControlSize::Large` | No |
| `.variant = TextFieldVariant::Filled` | `.variant = InputVariant::Filled` | No |

Because both new names alias existing enum types, *no duplicated fields, implicit
conversions, or overloads are needed*. Both spellings may be used in the same
translation unit; preserving the underlying type avoids symbol changes caused
by replacing an enum definition with a new one.

## For maintainers: review checklist

- Does this change preserve `Props` aggregate order and designated initializers?
- Are enum names/types consistent with existing control families?
- Does a new callback have clear invocation semantics and move-safe ownership?
- Are theme density, local control size and fixed layout dimensions independent?
- Do direct-header includes and `<terion/terion.h>` both compile as C++20?
- Are old and preferred API spellings covered by CI with `-Werror`?
- Are known limitations documented rather than silently changing behavior?
