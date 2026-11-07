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
- UTF-8 decoder and preliminary codepoint display-width estimation. Full grapheme cluster handling, Unicode emoji sequences, and key-sequence parsing are **not implemented yet**.
- Cmdly integration will be an optional `Console` component in a later milestone.

## License

Apache-2.0. See `LICENSE`.
