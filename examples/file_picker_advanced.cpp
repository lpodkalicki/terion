/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <chrono>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>
#include <terion/terion.h>

using namespace terion;

int main()
{
    Application app(std::make_unique<AnsiTerminalBackend>());
    auto browser = std::make_shared<FilePickerAsyncBrowser>(FilePickerAsyncBrowser::Options{
        .cache_capacity = 12,.cache_ttl = std::chrono::seconds(15)});
    State<bool> picker_open(false);
    State<FilePickerMode> mode(FilePickerMode::File);
    State<std::filesystem::path> directory(std::filesystem::current_path());
    State<std::filesystem::path> selected_path({});
    State<std::string> path_input(directory.get().string());
    State<std::string> filename("notes.md");
    State<std::string> search("");
    State<bool> show_hidden(false);
    State<bool> allow_overwrite(false);
    State<std::string> status("Open a file, select a directory or choose a save destination.");

    // The worker does not modify State<> or call UI callbacks. Completion is
    // observed on the event-loop thread and causes a single redraw.
    app.addInterval(std::chrono::milliseconds(40),[&] {
        if (browser->consumeUpdates()) app.invalidate();
    });

    const auto open = [&](FilePickerMode next_mode) {
        mode.set(next_mode);
        selected_path.set({});
        search.set("");
        path_input.set(directory.get().string());
        allow_overwrite.set(false);
        picker_open.set(true);
    };

    app.render([&] {
        return Column({.children = {
            Typography({.text = "FilePicker 2.0  |  async filesystem browser",.variant = TypographyVariant::H2}),
            Row({.children = {
                Button({.text = "Open file",.on_click = [&] { open(FilePickerMode::File); }}),
                Button({.text = "Select directory",.on_click = [&] { open(FilePickerMode::Directory); }}),
                Button({.text = "Save as...",.on_click = [&] { open(FilePickerMode::SaveFile); }})
            },.layout = {.gap = 1}}),
            Typography({.text = status.get()}),
            Typography({.text = "Use breadcrumbs / Up / Go to navigate, Search to filter, Refresh to invalidate cache."}),
            Typography({.text = "Save mode returns a path; this example does not write a file."}),
            FilePicker({
                .open = picker_open.get(),
                .mode = mode.get(),
                .directory = directory.get(),
                .selected_path = selected_path.get(),
                .extensions = mode.get() == FilePickerMode::Directory ? std::vector<std::string>{} :
                    std::vector<std::string>{".md",".txt",".cpp",".h"},
                .search = search.get(),
                .title = mode.get() == FilePickerMode::SaveFile ? "Save file" :
                    mode.get() == FilePickerMode::Directory ? "Select directory" : "Open file",
                .key = "file_picker_advanced",
                .show_hidden = show_hidden.get(),
                .icon_style = FilePickerIconStyle::Symbols,
                .width = 76,
                .height = 12,
                .on_directory_change = [&](std::filesystem::path value) {
                    path_input.set(value.string());
                    directory.set(std::move(value));
                },
                .on_selection_change = [&](std::filesystem::path value) {
                    selected_path.set(std::move(value));
                },
                .on_search_change = [&](std::string value) { search.set(std::move(value)); },
                .on_show_hidden_change = [&](bool value) { show_hidden.set(value); },
                .on_confirm = [&](std::filesystem::path value) {
                    status.set((mode.get() == FilePickerMode::SaveFile ? "Save destination: " : "Selected: ") +
                        value.string());
                    picker_open.set(false);
                },
                .on_close = [&] { picker_open.set(false); },
                .async_browser = browser,
                .show_path_input = true,
                .path_input = path_input.get(),
                .on_path_input_change = [&](std::string value) { path_input.set(std::move(value)); },
                .filename = filename.get(),
                .on_filename_change = [&](std::string value) { filename.set(std::move(value)); },
                .allow_overwrite = allow_overwrite.get(),
                .on_refresh = [&] { app.invalidate(); },
                .on_allow_overwrite_change = [&](bool value) { allow_overwrite.set(value); }
            })
        },.layout = {.padding = 1,.gap = 1}});
    });
    return app.run();
}
