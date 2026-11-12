/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <terion/terion.h>

using namespace terion;

int main()
{
    Application app(std::make_unique<AnsiTerminalBackend>());
    State<bool> picker_open(false);
    State<bool> directory_mode(false);
    State<std::filesystem::path> directory(std::filesystem::current_path());
    State<std::filesystem::path> selected_path({});
    State<std::string> search("");
    State<bool> show_hidden(false);
    State<int> icon_mode(0);
    State<std::string> status("Choose a file or directory");
    app.render([&] {
        return Column({.children = {
            Typography({.text = "FilePicker example"}),
            Button({.text = "Pick a C++ file",.on_click = [&] {
                directory_mode.set(false);
                selected_path.set({});
                search.set("");
                picker_open.set(true);
            }}),
            Button({.text = "Pick directory",.on_click = [&] {
                directory_mode.set(true);
                selected_path.set({});
                search.set("");
                picker_open.set(true);
            }}),
            Button({.text = icon_mode.get() == 0 ? "Icons: Symbols" :
                icon_mode.get() == 1 ? "Icons: Emoji" : "Icons: ASCII",
                .on_click = [&] { icon_mode.set((icon_mode.get()+1)%3); }}),
            Typography({.text = status.get()}),
            FilePicker({.open = picker_open.get(),
                .mode = directory_mode.get() ? FilePickerMode::Directory : FilePickerMode::File,
                .directory = directory.get(),.selected_path = selected_path.get(),
                .extensions = directory_mode.get() ? std::vector<std::string>{} : std::vector<std::string>{".cpp",".h"},
                .search = search.get(),.title = "Select a path",.key = "file_picker",
                .show_hidden = show_hidden.get(),
                .icon_style = icon_mode.get() == 0 ? FilePickerIconStyle::Symbols :
                    icon_mode.get() == 1 ? FilePickerIconStyle::Unicode : FilePickerIconStyle::Ascii,
                .on_directory_change = [&](std::filesystem::path value) { directory.set(std::move(value)); },
                .on_selection_change = [&](std::filesystem::path value) { selected_path.set(std::move(value)); },
                .on_search_change = [&](std::string value) { search.set(std::move(value)); },
                .on_show_hidden_change = [&](bool value) { show_hidden.set(value); },
                .on_confirm = [&](std::filesystem::path value) {
                    status.set("Selected: " + value.string());
                    picker_open.set(false);
                },
                .on_close = [&] { picker_open.set(false); }})
        },.layout = {.padding = 2,.gap = 1}});
    });
    return app.run();
}
