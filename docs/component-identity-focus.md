# Component Identity & Focus (#139e)

`key` and `id` have distinct purposes:

- `key`: reconciliation identity among sibling components. It controls retained
  state and hook lifetime. The reconciler never uses `id` when matching nodes.
- `id`: public identity of a component. `Application::focus(id)`,
  `Application::hasFocus(id)`, and scoped shortcuts use this value. `id` does
  not change retained state. `Application::focusedId()` returns the public id
  of the active control, or an empty string if it is unnamed.
  `Application::hasFocusedControl()` checks focus even for unnamed controls.
- `RenderNode::id()` is an internal numeric runtime identity, used for event
  dispatch, mouse hit-testing, and `FocusManager`. It is **not** a public id.

```cpp
TextField({.key = "state-slot",.id = "hostname",
           .value = hostname,.on_change = setHostname});
app.focus("hostname");     // true when mounted and eligible for focus
app.focus("state-slot");   // false: key is not a focus target
```

`Component::identified("hostname")` is an alternative for dynamically
constructed components. All public component `Props` also accept `.id`.

Within one focus scope, public ids of focusable controls must be unique.
Duplicate ids throw `std::invalid_argument` during render. The same id in
separate scopes is permitted. Disabled, hidden, or inactive controls cannot be
focused. `FocusScope` and `Dialog` keep using internal runtime ids to isolate
focus; a scope with `.id = "workspace"` can be targeted by scoped shortcuts.

```cpp
app.addShortcut({.key = Key::F1},onHelp,
    {.scope = ShortcutScope::FocusScope,.target = "workspace"});
app.addShortcut({.key = Key::F2},onSelect,
    {.scope = ShortcutScope::FocusedControl,.target = "hostname"});
```

Form controls are assigned their `FormField::name` as public id unless the
control already has an explicit id. Form submit/reset buttons have ids
`form-submit` and `form-reset`. Reconciliation keys remain independent.

## Breaking changes

The former implicit `key` lookup through `Application::focus`, `hasFocus`, and
`ShortcutOptions::target` has been removed. Migrate callers to an explicit
`.id`, keeping `.key` only where stable reconciliation is required. The
underlying `FocusManager` still addresses internal runtime node ids and is
available for low-level event handling.
