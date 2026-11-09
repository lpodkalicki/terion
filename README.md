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
  Local state uses
  `useState(initial)` returns a value and setter; the setter accepts either a new value or
  an updater function. Slots are retained across keyed reorders and discarded on unmount.
  Hooks must be called unconditionally in the same order. See `examples/hooks.cpp`.
- `useRef(initial)` returns a stable shared pointer (mutating it does not request a redraw).
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
- `Fragment({.children = {...}})` groups sibling declarations without introducing a
  layout node. It also works when returned from `component(lambda)`; a top-level
  fragment uses one root column host. See `examples/fragment.cpp`.
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
