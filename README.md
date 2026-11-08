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
- UTF-8 decoder and preliminary codepoint display-width estimation. Full grapheme cluster handling, Unicode emoji sequences, and key-sequence parsing are **not implemented yet**.
- Cmdly integration will be an optional `Console` component in a later milestone.

## License

Apache-2.0. See `LICENSE`.
