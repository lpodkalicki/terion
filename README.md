# Terion

Modern C++20 terminal UI framework (early foundation preview).

## Build

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
./build/examples/terion_hello
```

Press Escape to exit the demo. Linux/macOS with a terminal supporting ANSI true color.

## Design

- Public rendering primitives: `Cell`, `ScreenBuffer`, `Painter`, `DifferentialRenderer`.
- Terminal abstraction: `TerminalBackend`, `AnsiTerminalBackend`, `HeadlessTerminalBackend`.
- `Application` supports both low-level paint callbacks and a first declarative `Component` tree.
- `RenderNode` and `Reconciler` retain keyed node identity across view rebuilds.
- User-defined components use `inline const auto Counter = component([] { ... });`.
  Calling `Counter()` creates a deferred component declaration; the reconciler invokes its
  body with the correct hook and context scope. Pass props as lambda arguments, and use
  `Counter(props).keyed("stable-key")` for keyed lists. See `examples/hooks.cpp` and
  `examples/context.cpp`.
  For reusable components with larger props, prefer `component([](const MyProps& props) {
  ... })`. Declarations capture an owned snapshot of the arguments before deferred
  rendering; `.keyed("id")` moves an rvalue declaration instead of copying its tree.
  Props should own their referenced data (do not retain dangling `string_view` or pointers).
  Local state uses
  `useState(initial)` returns a value and setter; the setter accepts either a new value or
  an updater function. Slots are retained across keyed reorders and discarded on unmount.
  Hooks must be called unconditionally in the same order. See `examples/hooks.cpp`.
- `useRef(initial)` returns a stable shared pointer (mutating it does not request a redraw).
- `useReducer(reducer, initial)` returns `[state, dispatch]` for state transitions based on
  actions. `dispatch(action)` computes from the latest state; reducer slots follow the same
  keyed lifecycle as `useState`. See `examples/reducer.cpp`.
- `useId()` produces a unique, stable string for a mounted component's hook slot.
  It survives rerenders and keyed reordering, and changes after unmount/remount.
  These are runtime ids (not deterministic across program runs). See `examples/use_id.cpp`.
  `useMemo(factory, deps...)` and `useCallback(callback, deps...)` cache their results
  while dependencies compare equal. `useEffect(effect, deps...)` runs after the frame;
  a returned cleanup callback runs before a changed effect and on unmount. With no
  dependencies, the effect runs on every frame; pass `std::tuple{}` as a stable
  dependency to run it only on mount.
- `Context<T>` and `ContextProvider(context, value, child, key)` provide scoped values
  to descendants without passing props through intermediate components. A mounted
  function component reads the nearest provider using `useContext(context)`; outside
  providers it receives the context's fallback. Nested providers override their
  ancestors without affecting siblings. See `examples/context.cpp`.
- `ContextProvider(context, value, std::vector<Component>{...})` shares one context
  across several direct children without adding a layout node. A custom hook such as
  `useTheme()` can simply return `useContext(themeContext)`; a provider component can
  combine `useState()` with context data and an update callback. See `examples/stateful_context.cpp`.
- `Fragment({.children = {...}})` groups sibling declarations without introducing a
  layout node. It also works when returned from `component(lambda)`; a top-level
  fragment uses one root column host. See `examples/fragment.cpp`.
- `ErrorBoundary({.child = Panel(),.fallback = [](const std::string& message) {
  return Typography({.text = message}); }})` catches standard exceptions raised while
  expanding its descendant components and displays a fallback instead. A subsequent
  update retries the child. Errors raised by the fallback itself still propagate.
  See `examples/error_boundary.cpp`.
- `Dialog({.open = visible,.title = "Settings",.children = {SettingsForm()}})`
  renders arbitrary nested components in a modal overlay with scoped keyboard focus.
  Dialogs with custom children omit the built-in Confirm/Cancel footer by default;
  set `.showActions = true` to retain it. Dialogs without children preserve their
  built-in actions. See `examples/dialog_form.cpp`.
- Long dialog content scrolls within a clipped viewport using the mouse wheel,
  PageUp/PageDown, Home/End, or Up/Down when no child handles the key. Focused
  controls are automatically scrolled into view; the header and action footer
  remain fixed. Its scrollbar uses the same themed track/thumb glyphs as
  `ScrollView` and supports clicking the track and dragging the thumb.
- `DialogManager` provides imperative `open(Dialog::Props)` and `close()` for a
  single managed modal. Wrap the UI with `DialogProvider(manager,root)` and call
  `useDialogManager()` from descendant function components; changes automatically
  update the modal host. See `examples/dialog_manager_scroll.cpp` for a long
  document rendered inside a scrollable managed dialog.
- `ThemeProvider(theme,child)` applies a theme to its entire descendant subtree,
  including dialog overlays; nested providers override the nearest ancestor,
  and `useTheme()` returns the nearest provider's theme in function components.
  Nine presets are available: `dark`, `light`, `ocean`, `forest`, `sunset`,
  `violet`, `nord`, `dracula`, and `solarized`. See `examples/themes.cpp`.
- Semantic palette tokens `success`, `warning`, `error`, and `info` (plus matching
  `on_*` foregrounds) are available in every theme. `Button`, `Typography`, and
  `Progress` accept `.color = ColorVariant::Success` (or `Warning`, `Error`, `Info`,
  `Primary`), while existing components retain their default appearance.
  See `examples/semantic_colors.cpp`.
- `Checkbox`, `Switch`, `Radio`, `RadioGroup`, and `Slider` also accept `.color`,
  using the same semantic tokens for checked, selected, focused, and active
  indicators. Disabled controls retain the muted theme color.
- `When(visible, Widget())` conditionally includes a component without mounting its
  hooks when hidden. `ForEach(items, [](const auto& item) { return RowWidget(item).keyed(item.id); })`
  expands a range into siblings and requires unique nonempty keys for reliable state retention.
  See `examples/composition_helpers.cpp`.
- Initial constraints-based layout supports text, `Row`, `Column`, `Box`, padding, gap and fixed dimensions.
- Reactive `State<T>` automatically tracks dependencies in render callbacks; unchanged state does not trigger redraw.
- MUI-inspired components in `<terion/components/>`: `Typography`, `Box`, `Row`, `Column` and interactive `Button`.
- SGR mouse reporting (click, move, drag and wheel) with button hit-testing, hover and focus-on-click.
- Controlled `TextField` supports UTF-8 codepoint entry, cursor movement, Backspace/Delete and mouse focus.
- Drawing primitives, styled `Box`, `Divider`, `Progress` and button visual variants.
- Theme-aware `Paper` and `Card`, plus `Stack` and equal-column `Grid` layouts.
- `LayoutStyle` supports minimum/maximum dimensions, weighted `flex` in `Row`/`Column`, `justifyContent` and `alignItems`.
- `examples/fullscreen.cpp` demonstrates a terminal-sized dashboard with all current declarative components.
- Buttons have true-color surfaces, Unicode rounded borders, hover/focus states and compact/medium/large sizes (`examples/buttons.cpp`).
- Text fields have outlined/filled variants, themed focus/disabled states and horizontal text viewport (`examples/text_fields.cpp`).
- Nested painter clipping and vertical `ScrollView` with controlled wheel scrolling (`examples/scroll_view.cpp`).
- ScrollView shows a themed scrollbar and accepts arrows, PageUp/PageDown, Home/End when focused.
- Scrollbar thumb can be dragged with the left mouse button; clicking the track jumps to that position.
- Mouse pointer shapes use OSC 22 where supported: normal over surfaces, text over editable/text areas, hand over buttons, grab over scrollbars.
- `TerminalBackend::capabilities()` reports terminal features. OSC 22 pointer shapes are enabled by default for Kitty and Ghostty only; set `TERION_MOUSE_POINTER_SHAPE=1` to opt in or `=0` to disable them explicitly. Other mouse interactions work independently.
- Unified hover tracking highlights interactive buttons, editable text fields and scrollbar thumbs, including terminals without OSC 22 pointer shapes.
- Controlled `Select` with an overlay menu, keyboard navigation, mouse selection, wheel scrolling and disabled options (`examples/select.cpp`).
- `OverlayManager` collects non-owning `Portal` entries after reconciliation, paints them above the component tree in z-index order, routes popup mouse events and dismisses inactive menus.
- `Select` menus choose the available space above/below the field, open near the selected option and show a draggable scrollbar for long lists. The example contains two independent selects.
- Controlled modal `Dialog` overlays the screen with a dimmed backdrop, title, message and Cancel/Confirm actions; it intercepts keyboard/mouse events while open (`examples/dialog.cpp`).
- Event handlers support capture → target → bubble propagation (`onEventCapture`, `onEvent`) with `stopPropagation()` and `preventDefault()`.
- `FocusScope` confines keyboard focus to one active subtree (for example a form). An open `Dialog` automatically creates a modal focus scope and restores the previously focused background element when closed.
- `Dialog::Props::children` accepts arbitrary nested components; the overlay lays them out and paints them inside the dialog. See `examples/architecture.cpp` for a composed dialog with a `TextField` and a `Button`.
- UTF-8 decoder and preliminary codepoint display-width estimation. Full grapheme cluster handling, Unicode emoji sequences, and key-sequence parsing are **not implemented yet**.
- Cmdly integration will be an optional `Console` component in a later milestone.

## License

Apache-2.0. See `LICENSE`.
