/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <algorithm>
#include <array>
#include <memory>
#include <string>
#include <utility>
#include <terion/terion.h>

using namespace terion;

namespace {

constexpr std::array<Key,12> function_keys{
    Key::F1,Key::F2,Key::F3,Key::F4,Key::F5,Key::F6,
    Key::F7,Key::F8,Key::F9,Key::F10,Key::F11,Key::F12
};

std::string activeControl(const Application& app)
{
    if (app.hasFocus("editor")) return "Markdown editor";
    if (app.hasFocus("search")) return "Search field";
    if (app.hasFocus("action")) return "Action button";
    if (app.hasFocus("focus-editor")) return "Focus editor button";
    if (app.hasFocus("focus-search")) return "Focus search button";
    if (app.hasFocus("focus-action")) return "Focus action button";
    return app.focused().empty() ? "(none)" : "Other control";
}

} /* End of anonymous namespace */

int main()
{
    Application app(std::make_unique<AnsiTerminalBackend>());
    const std::string initial_text =
        "The editor has its own shortcuts.\n"
        "Try Ctrl+A, Ctrl+C, Ctrl+V and Ctrl+Z.\n"
        "Ctrl+K runs only while this editor is focused.\n"
        "Use Tab / Shift+Tab to change focus.\n"
        "F2, F3 and F4 jump directly to a control.";
    State<std::string> editor{initial_text};
    State<std::string> search{"Type here after pressing F3"};
    State<std::string> status{"Ready: press F1 for help"};
    State<std::string> last_function{"(none)"};
    State<int> function_count{0};
    State<bool> show_help{false};

    const auto focus_to = [&](const char* key, const char* description) {
        status.set(app.focus(key) ? std::string("Focused: ") + description :
            std::string("Cannot focus: ") + description);
    };

    // F1-F12 have explicit bindings and are handled before focused text controls.
    // This guarantees the demo receives function keys even while editing text.
    for (std::size_t i = 0; i < function_keys.size(); ++i)
    {
        const int number = static_cast<int>(i)+1;
        app.addShortcut({.key = function_keys[i]},[&,number] {
            last_function.set("F"+std::to_string(number));
            function_count.set(function_count.get()+1);
            switch (number)
            {
                case 1:
                    show_help.set(!show_help.get());
                    status.set("F1: help toggled");
                    break;
                case 2: focus_to("editor","Markdown editor"); break;
                case 3: focus_to("search","Search field"); break;
                case 4: focus_to("action","Action button"); break;
                case 5:
                    editor.set(initial_text);
                    search.set("Type here after pressing F3");
                    status.set("F5: sample values reset");
                    break;
                case 6:
                    status.set(app.focus().next() ? "F6: focus next" : "F6: no focus target");
                    break;
                case 7:
                    status.set(app.focus().previous() ? "F7: focus previous" : "F7: no focus target");
                    break;
                default:
                    status.set("F"+std::to_string(number)+": function key received");
                    break;
            }
        },{.priority = ShortcutPriority::BeforeFocused});
    }

    // The editor keeps Ctrl+C/V/Z; global shortcuts run only if the control
    // does not consume the event.
    app.addShortcut({.key = Key::Character,.character = U's',.ctrl = true},
        [&] { status.set("Ctrl+S: save requested (demo only)"); });
    app.addShortcut({.key = Key::Character,.character = U'k',.ctrl = true},
        [&] { status.set("Ctrl+K: editor-only shortcut"); },
        {.scope = ShortcutScope::FocusedControl,.target = "editor"});
    app.addShortcut({.key = Key::Character,.character = U'r',.ctrl = true},
        [&] { status.set("Ctrl+R: workspace-scoped shortcut"); },
        {.scope = ShortcutScope::FocusScope,.target = "workspace"});

    app.render([&] {
        const Size size = app.backend().size();
        const int width = std::max(1,size.width-2);
        const bool narrow = width < 70;
        const int editor_width = narrow ? width : std::max(20,(width*3)/5-1);
        const int sidebar_width = narrow ? width : std::max(15,width-editor_width-2);
        const int editor_height = std::max(4,size.height-(narrow ? 20 : 15));
        const auto focus_tag = [&](const char* key) -> std::string {
            return app.hasFocus(key) ? " < FOCUSED" : "";
        };
        const auto focus_buttons = [&] {
            return Row({.children = {
                Button({.text = "Editor",.key = "focus-editor",
                    .on_click = [&] { focus_to("editor","Markdown editor"); },
                    .size = ButtonSize::Compact}),
                Button({.text = "Search",.key = "focus-search",
                    .on_click = [&] { focus_to("search","Search field"); },
                    .size = ButtonSize::Compact}),
                Button({.text = "Action",.key = "focus-action",
                    .on_click = [&] { focus_to("action","Action button"); },
                    .size = ButtonSize::Compact})
            },.layout = {.gap = 1}});
        };
        const auto action_button = [&] {
            return Button({.text = "Run action [F4]",.key = "action",
                .on_click = [&] { status.set("Action button activated"); },
                .size = ButtonSize::Compact});
        };
        const auto editor_field = [&] {
            return TextArea({.value = editor.get(),.key = "editor",.soft_wrap = true,
                .layout = {.width = editor_width,.height = editor_height},
                .on_change = [&](std::string text) { editor.set(std::move(text)); }});
        };
        const auto search_field = [&] {
            return TextField({.value = search.get(),.key = "search",
                .layout = {.width = sidebar_width},
                .on_change = [&](std::string text) { search.set(std::move(text)); },
                .size = ButtonSize::Compact});
        };

        const auto make_body = [&]() -> Component {
            if (narrow)
            {
                return Column({.children = {
                    search_field(),focus_buttons(),action_button(),
                    Typography({.text = "Editor"+focus_tag("editor")}),editor_field()
                },.layout = {.width = width,.gap = 1}});
            }
            Component editor_panel = Column({.children = {
                Typography({.text = "EDITOR"+focus_tag("editor"),
                    .variant = TypographyVariant::Subtitle}),editor_field()
            },.layout = {.width = editor_width,.gap = 1}});
            Component controls_panel = Column({.children = {
                Typography({.text = "SEARCH"+focus_tag("search"),
                    .variant = TypographyVariant::Subtitle}),
                search_field(),
                focus_buttons(),action_button(),
                Typography({.text = "Active: "+activeControl(app)}),
                Typography({.text = "Last: "+last_function.get()+
                    " ("+std::to_string(function_count.get())+" times)"})
            },.layout = {.width = sidebar_width,.gap = 1}});
            return Row({.children = {std::move(editor_panel),std::move(controls_panel)},
                .layout = {.width = width,.gap = 2}});
        };
        Component body = make_body();

        return FocusScope({.children = {
            Column({.children = {
                Typography({.text = "Keyboard & Shortcut Lab (#135b)",
                    .variant = TypographyVariant::H2}),
                Typography({.text = "Tab / Shift+Tab: focus | F1: help | F2-F4: jump | F5: reset | F6/F7: next/prev"}),
                Divider({.layout = {.width = width}}),
                std::move(body),
                Typography({.text = "Status: "+status.get()}),
                Typography({.text = show_help.get() ?
                    "F8-F12: test | Ctrl+S: global | Ctrl+K: editor | Ctrl+R: workspace | Fn may be needed" :
                    "Last function: "+last_function.get()+" ("+
                    std::to_string(function_count.get())+" total) | F1 for help"})
            },.layout = {.width = size.width,.height = size.height,.padding = 1,.gap = 1}})
        },.key = "workspace",.active = true});
    });
    return app.run();
}
