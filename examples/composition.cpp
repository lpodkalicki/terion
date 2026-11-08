/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <terion/terion.h>

#include <functional>
#include <string>
#include <utility>
#include <vector>

using namespace terion;

namespace {

struct FileRowProps
{
    std::string name;
    std::string kind;
    std::string key;
    std::function<void()> onOpen;
};

// An ordinary C++ function is all that is needed for a stateless component.
Component FileRow(FileRowProps props)
{
    return Row({.children = {
        Typography({.text = props.kind,.key = "kind"}),
        Typography({.text = props.name,.key = "filename"}),
        Button({.text = "Open",.key = "open",.onClick = std::move(props.onOpen),
            .size = ButtonSize::Compact})
    },.key = std::move(props.key),.layout = {.gap = 2}});
}

struct FileListProps
{
    std::string title;
    std::vector<std::string> files;
    std::string key;
    std::function<void(const std::string&)> onOpen;
};

// Composition: FileList renders FileRow without knowing its internal structure.
Component FileList(FileListProps props)
{
    std::vector<Component> rows;
    rows.reserve(props.files.size());
    for (const auto& name : props.files)
    {
        rows.emplace_back(FileRow({.name = name,.kind = "FILE",.key = name,
            .onOpen = [onOpen = props.onOpen,name] { if (onOpen) onOpen(name); }}));
    }
    return Card({.title = std::move(props.title),.children = std::move(rows),
        .key = std::move(props.key)});
}

} /* End of anonymous namespace */

int main()
{
    Application app(std::make_unique<AnsiTerminalBackend>());
    State<std::string> selected("None");
    State<bool> show_archives(true);

    app.render([&] {
        std::vector<Component> content;
        content.emplace_back(Typography({.text = "Terion - reusable components & composition"}));
        content.emplace_back(Typography({.text = "Selected: " + selected.get()}));
        content.emplace_back(Button({.text = "Toggle archives",.key = "toggle",
            .onClick = [&] { show_archives.set(!show_archives.get()); }}));
        content.emplace_back(FileList({.title = "Documents",.files = {"report.pdf","notes.txt"},
            .key = "documents",.onOpen = [&](const std::string& name) { selected.set(name); }}));
        if (show_archives.get())
        {
            content.emplace_back(FileList({.title = "Archives",.files = {"backup.zip","logs.tar"},
                .key = "archives",.onOpen = [&](const std::string& name) { selected.set(name); }}));
        }
        return Column({.children = std::move(content),.layout = {.padding = 1,.gap = 1}});
    });
    return app.run();
}
