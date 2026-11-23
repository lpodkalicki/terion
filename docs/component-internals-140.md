# Component Internals 2.0 (#140)

Terion stores a component's **shared metadata** separately from its **typed
payloads**.  The old `Component::Props` contained more than one hundred unrelated
fields, including the contents of tables, input controls, dialogs, trees and
virtual lists.  Every component used to carry storage for those fields.

## Storage model

`Component::Props` now holds common information only:

- `type`, `key`, `id`: component kind, reconciliation identity and public identity
- `children`, `layout`, `theme_override`: tree and layout information
- `disabled`, `on_blur`, `on_event_capture`, `on_event`: shared state and handlers

Component-specific values are organized into independently allocated, typed
families: `TextData`, `InputData`, `ActionData`, `SurfaceData`, `ScrollData`,
`ChoiceData`, `TableData`, `VirtualListData`, `TreeData`, `SplitData`,
`OverlayData`, `ProgressData`, `SliderData`, `FormData` and
`RenderFunctionData`.

An owning `detail::DataSlot<T>` stores each optional family in a separate
allocation.  Empty slots contain only a null pointer.  Copying a `Component`
**deep-copies** the populated payloads; moving it transfers ownership.
Thus changing a copied component cannot mutate the original component's
`TableData` or `TreeData`.

The `Props::textData()` (and corresponding) accessors return a const reference
to the group's default value if it is not present. Mutable access creates the
group on demand.  This keeps `Reconciler` behavior unchanged when a property is
not set and allows `Form` to bind nested controls without downcasting.

```cpp
Button button({.text = "Save", .on_click = onSave});

const auto& props = button.props();
// props.text.has_value() == true
// props.action.has_value() == true
// props.table.has_value() == false

const auto& text = props.textData().text;
```

## API decisions

- This is a **breaking internal API change**. Code constructing `Component`
  directly must provide the typed groups, not flattened `Component::Props`
  fields. Component constructors such as `TextField({...})` and `Table({...})`
  remain declarative.
- **No legacy, flattened `Props` adapter** is provided.
- The grouping is by **behavioral family**, not by a one-to-one variant arm
  for every `Component::Type`. This permits some controls (e.g. `TextField`)
  to use multiple families, without duplicating shared attributes.
- `RenderNode` still retains its existing hot-path representation; refactoring
  its runtime fields into family-specific state can be addressed separately.
- The heap allocation trade-off favors smaller retained-tree nodes and clearer
  responsibilities over minimizing construction-time allocations.

## Tests

`component_data_test.cpp` verifies ownership, absence of unrelated groups,
copy independence, selected-value reconciliation, form binding, keys and IDs.
The existing tests for text input, tables and tree navigation remain relevant.

On one 64-bit Linux / GCC 14 toolchain, `sizeof(Component::Props)` changed from
1,992 bytes to 520 bytes (about 74% smaller). Sizes are ABI-dependent and
should not be used as a portable invariant.
