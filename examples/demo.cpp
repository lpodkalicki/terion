/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <algorithm>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <memory>
#include <sstream>
#include <string>
#include <thread>
#include <utility>
#include <vector>
#include <terion/terion.h>

using namespace terion;

namespace {

std::string currentTime()
{
    const auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    const auto* local = std::localtime(&now);
    std::ostringstream out;
    if (local) out << std::put_time(local,"%H:%M:%S");
    return out.str();
}

} /* End of namespace */

int main()
{
    Application app(std::make_unique<AnsiTerminalBackend>());
    // The clock only invalidates the frame; Terion performs differential rendering.
    std::jthread clock([&](std::stop_token stop) {
        while (!stop.stop_requested())
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            if (!stop.stop_requested()) app.invalidate();
        }
    });
    State<int> sidebar_width{24};
    State<int> scroll_offset{0};
    State<int> content_scroll{0};
    State<int> progress{65};
    State<std::string> page{"files"};
    State<std::string> node{"documents"};
    State<std::vector<std::string>> expanded{{"library","documents","media"}};
    State<std::string> name{"New collection"};
    State<std::string> filter{""};
    State<std::string> language{"en"};
    State<std::string> mode{"auto"};
    State<std::string> radio{"green"};
    State<std::string> selected_row{"manual"};
    State<std::string> last_action{"Ready"};
    State<bool> enabled{true};
    State<bool> checked{true};
    State<bool> unicode_checked{false};
    State<bool> emoji_checked{true};
    State<bool> dialog_open{false};
    State<bool> light_theme{false};
    State<bool> show_hidden{false};
    State<int> imported{0};

    const std::vector<TreeItem> navigation = {
        {.key = "library",.label = "My library",.children = {
            {.key = "documents",.label = "Documents",.children = {
                {.key = "reports",.label = "Reports"},
                {.key = "contracts",.label = "Contracts"}}},
            {.key = "media",.label = "Media",.children = {
                {.key = "images",.label = "Images"},
                {.key = "videos",.label = "Videos"}}},
            {.key = "archives",.label = "Archives"}}},
        {.key = "favorites",.label = "Favorites"},
        {.key = "recent",.label = "Recently used"},
        {.key = "trash",.label = "Trash"}
    };
    const std::vector<TableRow> rows = {
        {.key = "manual",.cells = {"terion-guide.pdf","PDF","2.4 MB","2026-11-07"}},
        {.key = "report",.cells = {"report-2026.xlsx","Sheet","845 KB","2026-11-06"}},
        {.key = "notes",.cells = {"notes.md","Text","12 KB","2026-11-05"}},
        {.key = "photo",.cells = {"cover.png","Image","5.1 MB","2026-11-04"}},
        {.key = "archive",.cells = {"backup.zip","Archive","18 MB","2026-11-01"}},
        {.key = "hidden",.cells = {".index","System","4 KB","2026-11-07"}}
    };

    app.render([&] {
        const Size screen = app.backend().size();
        const bool compact = screen.width < 76;
        const bool narrow = screen.width < 52;
        const bool short_screen = screen.height < 20;
        const int body_height = std::max(0,screen.height - 4);
        const int left_width = std::clamp(sidebar_width.get(),16,std::max(16,screen.width - 33));
        const int content_width = compact ? std::max(0,screen.width - 2) :
            std::max(0,screen.width - left_width - 4);
        const std::string active = page.get();

        Component overview = TabPanel({
            .children = {
                Typography({.text = "Library overview / File details",.variant = TypographyVariant::H2}),
                Typography({.text = "Selected file: " + selected_row.get()}),
                Grid({.children = {
                    Card({.title = "Storage used",.children = {
                        Typography({.text = "42% of demo quota"}),
                        Progress({.value = 0.42,.layout = {.width = 15}})
                    },.layout = {.padding = 1,.gap = 1}}),
                    Card({.title = "Indexed files",.children = {
                        Typography({.text = "68% indexed"}),
                        Progress({.value = 0.68,.layout = {.width = 15}})
                    },.layout = {.padding = 1,.gap = 1}})
                },.layout = {.width = std::max(0,content_width - 4),.gap = 1},
                    .columns = compact ? 1 : 2}),
                Paper({.children = {
                    Typography({.text = "Indexing progress (simulated)",.variant = TypographyVariant::Subtitle}),
                    Progress({.value = progress.get() / 100.0,.layout = {.width = 25}}),
                    Row({.children = {
                        Button({.text = "+10%",.key = "progress-add",.variant = ButtonVariant::Contained,
                            .onClick = [&] { progress.set(std::min(100,progress.get() + 10)); }}),
                        Button({.text = "Reset",.key = "progress-reset",
                            .onClick = [&] { progress.set(0); }})
                    },.layout = {.gap = 2}})
                },.layout = {.padding = 1,.gap = 1}}),
                Box({.children = {
                    Typography({.text = "Box / Row / Column / Stack / Grid"}),
                    Stack({.children = {
                        Typography({.text = "Declarative"}),
                        Typography({.text = "Retained tree"}),
                        Typography({.text = "Differential render"})
                    },.layout = {.gap = 2},.direction = StackDirection::Horizontal})
                },.layout = {.padding = 1,.gap = 1},.border = BorderStyle::Rounded})
            },.key = "overview-panel",.active = active == "details",
            .layout = {.padding = 1,.gap = 1}
        });

        Component controls = TabPanel({
            .children = {
                Typography({.text = "Library settings / Form controls",.variant = TypographyVariant::H2}),
                TextField({.value = name.get(),.placeholder = "Name",.key = "demo-name",
                    .onChange = [&](std::string value) { name.set(std::move(value)); }}),
                Row({.children = {
                    Select({.options = {{"English","en"},{"Polski","pl"},{"Deutsch","de"}},
                        .value = language.get(),.key = "language",.layout = {.width = 20},
                        .onChange = [&](std::string value) { language.set(std::move(value)); }}),
                    DropdownMenu({.label = "Actions",.options = {{"Save","save"},{"Export","export"},
                        {"Delete","delete"}},.key = "actions",
                        .onSelect = [&](std::string value) { last_action.set("Action: " + value); }})
                },.layout = {.gap = 2}}),
                Divider({.layout = {.width = std::max(0,content_width - 4)}}),
                Row({.children = {
                    Checkbox({.label = "Default",.key = "checkbox-default",.checked = checked.get(),
                        .onChange = [&](bool value) { checked.set(value); }}),
                    Checkbox({.label = "Unicode",.key = "checkbox-unicode",.checked = unicode_checked.get(),
                        .variant = CheckboxVariant::Unicode,
                        .onChange = [&](bool value) { unicode_checked.set(value); }}),
                    Checkbox({.label = "Emoji",.key = "checkbox-emoji",.checked = emoji_checked.get(),
                        .variant = CheckboxVariant::Emoji,
                        .onChange = [&](bool value) { emoji_checked.set(value); }})
                },.layout = {.gap = 2}}),
                Switch({.label = "Enabled",.key = "enabled",.checked = enabled.get(),
                    .onChange = [&](bool value) { enabled.set(value); }}),
                Row({.children = {
                    Radio({.label = "Green",.key = "radio-green",.checked = radio.get() == "green",
                        .onChange = [&](bool value) { if (value) radio.set("green"); }}),
                    Radio({.label = "Blue",.key = "radio-blue",.checked = radio.get() == "blue",
                        .onChange = [&](bool value) { if (value) radio.set("blue"); }})
                },.layout = {.gap = 2}}),
                RadioGroup({.options = {{"Automatic","auto"},{"Manual","manual"},{"Off","off"}},
                    .value = mode.get(),.key = "mode",
                    .onChange = [&](std::string value) { mode.set(std::move(value)); }}),
                Row({.children = {
                    Button({.text = "Open dialog",.key = "open-dialog",.variant = ButtonVariant::Contained,
                        .onClick = [&] { dialog_open.set(true); }}),
                    Button({.text = "Disabled",.disabled = true}),
                    Button({.text = "Text button",.key = "text-button",.variant = ButtonVariant::Text,
                        .onClick = [&] { last_action.set("Text button"); }})
                },.layout = {.gap = 2}}),
                Typography({.text = "Name: " + name.get() + " | Mode: " + mode.get()})
            },.key = "controls-panel",.active = active == "settings",
            .layout = {.padding = 1,.gap = 1}
        });

        Component data = TabPanel({
            .children = {
                Typography({.text = "Files / " + node.get(),.variant = TypographyVariant::H2}),
                Row({.children = {
                    Tooltip({.child = Button({.text = "Import",.key = "import-file",.variant = ButtonVariant::Contained,
                        .onClick = [&] {
                            imported.set(imported.get()+1);
                            const auto status = "Simulated import #"+std::to_string(imported.get());
                            last_action.set(status);
                            (void)app.toasts().enqueue({.message = status,
                                .severity = ToastSeverity::Success});
                        }}),.text = "Import files into the current collection",.key = "import-tooltip"}),
                    Button({.text = "File details",.key = "file-details",
                        .onClick = [&] { page.set("details"); }}),
                    Button({.text = "Delete...",.key = "delete-file",
                        .onClick = [&] { dialog_open.set(true); }})
                },.layout = {.gap = 1}}),
                TextField({.value = filter.get(),.placeholder = "Search files...",.key = "table-filter",
                    .onChange = [&](std::string value) { filter.set(std::move(value)); }}),
                ContextMenu({.child = Table({.columns = compact ? std::vector<TableColumn>{{"Name",22},{"Type",10}} :
                    std::vector<TableColumn>{{"Name",22},{"Type",10,TableAlignment::Center},
                        {"Size",10,TableAlignment::Right},{"Modified",14}},
                    .rows = rows,
                    .filter = [query = filter.get(),hidden = show_hidden.get()](const TableRow& row) {
                        return (hidden || row.key != "hidden") &&
                            (query.empty() || row.cells[0].find(query) != std::string::npos ||
                                row.cells[1].find(query) != std::string::npos);
                    },
                    .formatCell = [](const TableRow& row, std::size_t column) {
                        TableCellFormat cell{.text = row.cells[column]};
                        if (column == 1)
                        {
                            cell.foreground = row.cells[column] == "PDF" ? Color::rgb(255,185,65) :
                                row.cells[column] == "Image" ? Color::rgb(95,180,255) : Color::rgb(135,215,150);
                            cell.bold = true;
                        }
                        return cell;
                    },
                    .value = selected_row.get(),.key = "files-table",
                    .layout = {.height = std::max(3,std::min(12,body_height - 10))},
                    .onChange = [&](std::string value) { selected_row.set(std::move(value)); }}),
                    .options = {{"Open details","open"},{"Rename","rename"},
                        {"Delete...","delete"}},.key = "file-context-menu",
                    .onSelect = [&](std::string action) {
                        if (action == "open") page.set("details");
                        else if (action == "delete") dialog_open.set(true);
                        last_action.set("Context menu: " + action + " (" + selected_row.get() + ")");
                    }}),
                Checkbox({.label = "Show hidden files",.key = "show-hidden",.checked = show_hidden.get(),
                    .onChange = [&](bool value) { show_hidden.set(value); }}),
                Typography({.text = "Selected: " + selected_row.get() + " | Demo imports: " +
                    std::to_string(imported.get())})
            },.key = "data-panel",.active = active == "files",
            .layout = {.padding = 1,.gap = 1}
        });

        Component layout = TabPanel({
            .children = {
                Typography({.text = "Layout / ScrollView / Composition",.variant = TypographyVariant::H2}),
                Typography({.text = "Drag the main separator to resize the sidebar."}),
                Paper({.children = {
                    Typography({.text = "Scrollable log",.variant = TypographyVariant::Subtitle}),
                    ScrollView({.children = {
                        Typography({.text = "[10:00] Application started"}),
                        Typography({.text = "[10:01] Receiver connected"}),
                        Typography({.text = "[10:02] Tracker synchronized"}),
                        Typography({.text = "[10:03] Data stream running"}),
                        Typography({.text = "[10:04] Table refreshed"}),
                        Typography({.text = "[10:05] No errors reported"}),
                        Typography({.text = "[10:06] Monitoring active"}),
                        Typography({.text = "[10:07] Ready"})
                    },.key = "demo-log",.layout = {.width = 45,.height = 7,.gap = 0},
                        .scrollOffset = scroll_offset.get(),
                        .onScroll = [&](int value) { scroll_offset.set(value); }})
                },.layout = {.padding = 1,.gap = 1}}),
                Typography({.text = "ResizableSplit, TreeView and Tabs compose the fullscreen shell."}),
                Button({.text = "Back to files",.key = "back-overview",
                    .onClick = [&] { page.set("files"); }})
            },.key = "layout-panel",.active = active == "showcase",
            .layout = {.padding = 1,.gap = 1}
        });

        const auto makeFolderTree = [&]() -> Component { return Paper({.children = {
            Typography({.text = "COLLECTIONS",.variant = TypographyVariant::Subtitle}),
            TreeView({.items = navigation,.expanded = expanded.get(),.value = node.get(),
                .key = "navigation-tree",.layout = {.height = std::max(3,body_height - 5)},
                .onChange = [&](std::string value) {
                    node.set(std::move(value));
                    page.set("files");
                    content_scroll.set(0);
                },
                .onToggle = [&](std::string key, bool open) {
                    auto values = expanded.get();
                    values.erase(std::remove(values.begin(),values.end(),key),values.end());
                    if (open) values.push_back(std::move(key));
                    expanded.set(std::move(values));
                }})
        },.layout = {.padding = 1,.gap = 1}}); };

        Component folders = TabPanel({.children = compact ? std::vector<Component>{makeFolderTree()} :
            std::vector<Component>{},
            .key = "folders-panel",.active = compact && active == "folders",
            .layout = {.padding = 0}});
        // The narrow layout keeps the folder tree as a page, preserving keyboard access.
        Component content = Column({.children = {
            Tabs({.options = compact ? std::vector<TabOption>{{"Folders","folders"},{"Files","files"},
                {"Details","details"},{"Settings","settings"},{"More","showcase"}} :
                std::vector<TabOption>{{"Files","files"},{"Details","details"},
                    {"Settings","settings"},{"Showcase","showcase"}},
                .value = active,.key = "demo-tabs",
                .onChange = [&](std::string value) {
                    page.set(std::move(value));
                    content_scroll.set(0);
                }}),
            ScrollView({.children = {std::move(folders),std::move(data),std::move(overview),
                std::move(controls),std::move(layout)},
                .key = "content-scroll",.layout = {.width = content_width,.gap = 0,.flex = 1},
                .scrollOffset = content_scroll.get(),
                .onScroll = [&](int value) { content_scroll.set(value); }})
        },.layout = {.width = compact ? screen.width :
            std::max(0,screen.width-left_width-1),.height = body_height,.gap = 0,.flex = 1}});

        Component body = compact ? Component(std::move(content)) :
            Component(ResizableSplit({.children = {makeFolderTree(),std::move(content)},
                .direction = SplitDirection::Horizontal,.position = left_width,
                .minFirst = 16,.minSecond = 28,.key = "demo-split",
                .layout = {.width = screen.width,.height = body_height},
                .onChange = [&](int value) { sidebar_width.set(value); }}));

        std::vector<Component> menus;
        menus.emplace_back(DropdownMenu({.label = "File",.options = {{"Import","import"},
            {"New collection","new"},{"Exit","exit"}},.key = "top-file",
            .onSelect = [&](std::string value) {
                last_action.set("File: "+value);
                if (value == "exit") app.quit();
                else if (value == "import") imported.set(imported.get()+1);
                else if (value == "new") page.set("settings");
            }}));
        if (!narrow)
            menus.emplace_back(DropdownMenu({.label = "Edit",.options = {{"Rename","rename"},
                {"Delete...","delete"}},.key = "top-edit",
                .onSelect = [&](std::string value) {
                    last_action.set("Edit: "+value);
                    if (value == "delete") dialog_open.set(true);
                    else page.set("details");
                }}));
        menus.emplace_back(DropdownMenu({.label = "View",.options = {{"Folders","folders"},
            {"Files","files"},{"Details","details"},{"Settings","settings"},
            {"Showcase","showcase"}},.key = "top-view",
            .onSelect = [&](std::string value) { page.set(std::move(value)); }}));
        if (!narrow)
            menus.emplace_back(DropdownMenu({.label = "Help",.options = {{"About","about"}},.key = "top-help",
                .onSelect = [&](std::string) { last_action.set("Terion C++20 / v1.0 demo"); }}));

        return Column({
            .children = {
                Paper({.children = {Row({.children = {
                    Typography({.text = narrow ? "TERION" : "TERION / File Library v1.0",
                        .variant = TypographyVariant::H2}),
                    Row({.children = std::move(menus),.layout = {.gap = narrow ? 0 : 1}}),
                    Typography({.text = currentTime()})
                },.layout = {.width = screen.width,.justifyContent = JustifyContent::SpaceBetween}})},
                    .layout = {.width = screen.width,.padding = 0}}),
                Divider({.layout = {.width = screen.width}}),
                std::move(body),
                Divider({.layout = {.width = screen.width}}),
                Paper({.children = {Row({.children = {
                    Typography({.text = short_screen || narrow ? "View: " + active + " | " + last_action.get() :
                        "Ready | Tab/Shift+Tab: focus | Esc: close | View: " + active +
                        " | " + last_action.get()}),
                    Button({.text = light_theme.get() ? "Dark" : "Light",.key = "theme-toggle",
                        .variant = ButtonVariant::Text,.onClick = [&] {
                            light_theme.set(!light_theme.get());
                            app.setTheme(light_theme.get() ? Theme::light() : Theme::dark());
                        },.size = ButtonSize::Compact})
                },.layout = {.width = screen.width,.justifyContent = JustifyContent::SpaceBetween}})},
                    .layout = {.width = screen.width,.padding = 0}}),
                Dialog({.open = dialog_open.get(),.title = "Delete file?",
                    .message = "Demo only: no real files will be removed.",.key = "demo-dialog",
                    .confirmText = "Delete",.cancelText = "Cancel",
                    .onConfirm = [&] {
                        last_action.set("Simulated delete: "+selected_row.get());
                        (void)app.toasts().enqueue({.message = "File deleted (demo)",
                            .severity = ToastSeverity::Success});
                        dialog_open.set(false);
                    },
                    .onClose = [&] { dialog_open.set(false); }})
            },
            .layout = {.width = screen.width,.height = screen.height,.gap = 0,
                .alignItems = AlignItems::Stretch}
        });
    });
    return app.run();
}
