/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#ifndef TERION_COMPONENTS_FILE_PICKER_H
#define TERION_COMPONENTS_FILE_PICKER_H

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>
#include <terion/components/button.h>
#include <terion/components/breadcrumbs.h>
#include <terion/components/checkbox.h>
#include <terion/components/file_picker_browser.h>
#include <terion/components/file_picker_async.h>
#include <terion/components/column.h>
#include <terion/components/dialog.h>
#include <terion/components/row.h>
#include <terion/components/text_field.h>
#include <terion/components/typography.h>
#include <terion/components/virtual_list.h>

namespace terion {

// Controlled modal file picker. The parent owns directory, selection and search text;
// callbacks should update that state and trigger Application reconciliation.
class FilePicker : public Component
{
public:
    struct Props
    {
        bool open = false;
        FilePickerMode mode = FilePickerMode::File;
        std::filesystem::path directory = {};
        std::filesystem::path selected_path = {};
        std::vector<std::string> extensions = {};
        std::string search = {};
        std::string title = "Select file";
        std::string key = {};
        bool show_hidden = false;
        FilePickerIconStyle icon_style = FilePickerIconStyle::Symbols;
        bool show_breadcrumbs = true;
        bool show_entry_count = true;
        bool show_selection_info = true;
        int width = 64;
        int height = 16;
        std::function<void(std::filesystem::path)> on_directory_change = {};
        std::function<void(std::filesystem::path)> on_selection_change = {};
        std::function<void(std::string)> on_search_change = {};
        std::function<void(bool)> on_show_hidden_change = {};
        std::function<void(std::filesystem::path)> on_confirm = {};
        std::function<void()> on_close = {};
        // Optional non-blocking filesystem provider. Without it, FilePicker
        // retains its original synchronous behaviour for compatibility.
        std::shared_ptr<FilePickerAsyncBrowser> async_browser = {};
        bool show_path_input = false;
        std::string path_input = {};
        std::function<void(std::string)> on_path_input_change = {};
        std::string filename = {};
        std::function<void(std::string)> on_filename_change = {};
        bool allow_overwrite = false;
        std::function<void()> on_refresh = {};
        std::function<void(bool)> on_allow_overwrite_change = {};
    }; /* End of struct Props */

    explicit FilePicker(Props props) : Component(create(std::move(props))) {}

private:
    [[nodiscard]] static std::vector<Breadcrumbs::Item> breadcrumbItems(
        const std::filesystem::path& path,int width)
    {
        std::vector<Breadcrumbs::Item> result;
        std::filesystem::path current;
        for (const auto& part : path)
        {
            current /= part;
            const auto label = part == path.root_path() ? std::string("/") : part.string();
            if (!label.empty()) result.push_back({.label = label,.value = current.string()});
        }
        if (result.empty()) result.push_back({.label = ".",.value = path.string()});
        // Keep the root and as much of the end as can fit. Dropped parents
        // remain reachable through the Up button or editable path input.
        const int budget = std::max(16,width-12);
        auto total = [&] {
            int length = 0;
            for (const auto& item : result) length += static_cast<int>(item.label.size())+3;
            return length;
        };
        bool shortened = false;
        while (result.size() > 2 && total() > budget)
        {
            result.erase(result.begin()+1);
            shortened = true;
        }
        if (shortened)
            result.insert(result.begin()+1,{.label = "...",.value = "",.disabled = true});
        return result;
    }

    [[nodiscard]] static bool validFilename(const std::string& name)
    {
        if (name.empty() || name == "." || name == "..") return false;
        const auto path = std::filesystem::path(name);
        return !path.is_absolute() && !path.has_parent_path() &&
               path.filename() == path && name.find('/') == std::string::npos &&
               name.find('\\') == std::string::npos;
    }

    static Component create(Props props)
    {
        std::error_code error;
        auto directory = props.directory;
        if (directory.empty()) directory = std::filesystem::current_path(error);
        if (error) directory.clear();

        std::shared_ptr<const FilePickerListing> listing;
        bool loading = false;
        bool directory_ready = false;
        if (props.open && !directory.empty())
        {
            const FilePickerBrowser::Options options{
                .extensions = props.extensions,.search = props.search,.show_hidden = props.show_hidden};
            if (props.async_browser)
            {
                auto snapshot = props.async_browser->request(directory,options);
                listing = std::move(snapshot.listing);
                loading = snapshot.loading;
                directory_ready = snapshot.ready && listing && listing->error.empty();
            }
            else
            {
                listing = std::make_shared<FilePickerListing>(FilePickerBrowser::list(directory,options));
                directory_ready = listing->error.empty();
            }
        }
        if (!listing)
        {
            auto empty = std::make_shared<FilePickerListing>();
            empty->directory = directory;
            if (props.open && directory.empty()) empty->error = "Cannot determine current directory";
            listing = std::move(empty);
        }

        int selected_index = -1;
        for (std::size_t index = 0; index < listing->entries.size(); ++index)
            if (listing->entries[index].path == props.selected_path)
            {
                selected_index = static_cast<int>(index);
                break;
            }

        const auto on_directory_change = props.on_directory_change;
        const auto on_selection_change = props.on_selection_change;
        const auto on_search_change = props.on_search_change;
        const auto on_confirm = props.on_confirm;
        const auto on_close = props.on_close;
        const auto mode = props.mode;
        const auto selected_path = props.selected_path;
        const bool has_parent = directory.has_parent_path() && directory.parent_path() != directory;

        const auto navigate = [on_directory_change,on_selection_change,on_search_change](
            const std::filesystem::path& path) {
            if (on_selection_change) on_selection_change({});
            if (on_search_change) on_search_change("");
            if (on_directory_change) on_directory_change(path);
        };

        const auto activate = [listing,navigate,on_confirm,mode,
                               on_filename_change = props.on_filename_change](const int index) {
            if (index < 0 || static_cast<std::size_t>(index) >= listing->entries.size()) return;
            const auto& entry = listing->entries[static_cast<std::size_t>(index)];
            if (entry.is_directory) navigate(entry.path);
            else if (mode == FilePickerMode::File && on_confirm) on_confirm(entry.path);
            else if (mode == FilePickerMode::SaveFile && on_filename_change)
                on_filename_change(entry.name);
        };

        bool can_confirm = false;
        std::filesystem::path result;
        if (mode == FilePickerMode::Directory)
        {
            result = directory;
            std::error_code directory_error;
            if (!selected_path.empty() && selected_path.parent_path() == directory &&
                std::filesystem::is_directory(selected_path,directory_error) && !directory_error)
                result = selected_path;
            directory_error.clear();
            can_confirm = std::filesystem::is_directory(result,directory_error) && !directory_error;
        }
        else if (!selected_path.empty() && selected_index >= 0)
        {
            std::error_code file_error;
            can_confirm = std::filesystem::is_regular_file(selected_path,file_error) && !file_error;
            result = selected_path;
        }

        if (mode == FilePickerMode::SaveFile)
        {
            if (directory_ready && validFilename(props.filename) && !directory.empty())
            {
                result = directory / props.filename;
                std::error_code exists_error;
                const bool exists = std::filesystem::exists(result,exists_error);
                const bool is_directory = exists && std::filesystem::is_directory(result,exists_error);
                can_confirm = !exists_error && !is_directory && (!exists || props.allow_overwrite);
            }
        }

        const bool selected_directory = selected_index >= 0 &&
            listing->entries[static_cast<std::size_t>(selected_index)].is_directory;
        std::vector<Component> children;
        children.reserve(8);
        if (props.show_breadcrumbs)
            children.emplace_back(Breadcrumbs({.items = breadcrumbItems(directory,props.width),
                .key = "file_picker_location",.on_navigate = [navigate](std::string value) {
                    if (!value.empty()) navigate(std::filesystem::path(std::move(value)));
                }}));
        else
            children.emplace_back(Typography({.text = directory.string(),
                .key = "file_picker_location",.color = ColorVariant::Info,
                .style = {.bold = true}}));
        if (props.show_path_input)
        {
            children.emplace_back(Row({.children = {
                TextField({.value = props.path_input,.placeholder = "Enter directory path...",
                    .key = "file_picker_path_input",.layout = {.width = std::max(12,props.width-16)},
                    .on_change = props.on_path_input_change}),
                Button({.text = "Go",.key = "file_picker_go",
                    .on_click = [navigate,directory,input = props.path_input] {
                        if (input.empty()) return;
                        const auto entered = std::filesystem::path(input);
                        navigate((entered.is_absolute() ? entered : directory / entered).lexically_normal());
                    }})
            },.layout = {.gap = 1}}));
        }
        children.emplace_back(Row({.children = {
            Button({.text = "Up",.key = "file_picker_parent",.disabled = !has_parent,
                .on_click = [directory,navigate] { navigate(directory.parent_path()); }}),
            Button({.text = "Open folder",.key = "file_picker_open_folder",
                .disabled = !selected_directory,
                .on_click = [selected_path,navigate] { navigate(selected_path); }}),
            Button({.text = props.show_hidden ? "Hide dotfiles" : "Show dotfiles",
                .key = "file_picker_hidden",.disabled = !props.on_show_hidden_change,
                .on_click = [callback = props.on_show_hidden_change,hidden = props.show_hidden] {
                    if (callback) callback(!hidden);
                }}),
            Button({.text = "Refresh",.key = "file_picker_refresh",
                .on_click = [browser = props.async_browser,directory,
                             callback = props.on_refresh] {
                    if (browser) browser->invalidate(directory);
                    if (callback) callback();
                }})
        },.layout = {.gap = 1}}));
        children.emplace_back(TextField({.value = props.search,.placeholder = "Search filenames...",
            .key = "file_picker_search",.layout = {.width = std::max(12,props.width-6)},
            .on_change = on_search_change}));
        if (loading && listing->entries.empty())
            children.emplace_back(Typography({.text = "Loading directory...",
                .key = "file_picker_help",.color = ColorVariant::Info}));
        else if (!listing->error.empty())
            children.emplace_back(Typography({.text = "Filesystem error: " + listing->error,
                .key = "file_picker_error",.color = ColorVariant::Error}));
        else
            children.emplace_back(Typography({
                .text = (loading ? "Refreshing...  |  " : "") +
                    (props.show_entry_count ? std::to_string(listing->entries.size()) + " entries  |  " : "") +
                    "Enter: open  |  Double-click: open",
                .key = "file_picker_help",.color = ColorVariant::Info}));
        children.emplace_back(VirtualList({.item_count = static_cast<int>(listing->entries.size()),
            .item_text = [listing,icon_style = props.icon_style](int index) {
                const auto& entry = listing->entries.at(static_cast<std::size_t>(index));
                std::string_view prefix;
                switch (icon_style)
                {
                case FilePickerIconStyle::Unicode:
                    prefix = entry.is_directory ? "  📁    " : "  ·     ";
                    break;
                case FilePickerIconStyle::Ascii:
                    prefix = entry.is_directory ? "  [DIR]   " : "  [FILE]  ";
                    break;
                case FilePickerIconStyle::Symbols:
                    prefix = entry.is_directory ? "  ▸   " : "  ·   ";
                    break;
                case FilePickerIconStyle::None:
                    break;
                }
                return std::string(prefix) + entry.name;
            },
            .item_styled = [listing,icon_style = props.icon_style](int index) -> StyledText {
                const auto& entry = listing->entries.at(static_cast<std::size_t>(index));
                StyledText result;
                switch (icon_style)
                {
                case FilePickerIconStyle::Unicode:
                    result.push_back({.text = entry.is_directory ? "  📁    " : "  ·     ",
                        .color = entry.is_directory ? ColorVariant::Info : ColorVariant::Default});
                    break;
                case FilePickerIconStyle::Ascii:
                    result.push_back({.text = entry.is_directory ? "  [DIR]   " : "  [FILE]  ",
                        .color = entry.is_directory ? ColorVariant::Info : ColorVariant::Default});
                    break;
                case FilePickerIconStyle::Symbols:
                    result.push_back({.text = entry.is_directory ? "  ▸   " : "  ·   ",
                        .color = entry.is_directory ? ColorVariant::Info : ColorVariant::Default});
                    break;
                case FilePickerIconStyle::None:
                    break;
                }
                result.push_back({.text = entry.name + (entry.is_directory ? "/" : ""),
                    .color = entry.is_directory ? ColorVariant::Info : ColorVariant::Default,
                    .bold = entry.is_directory});
                return result;
            },
            .selected_index = selected_index,
            .row_height = 1,
            .on_select = [listing,on_selection_change,mode,
                          on_filename_change = props.on_filename_change](const int index) {
                if (index < 0 || static_cast<std::size_t>(index) >= listing->entries.size()) return;
                const auto& entry = listing->entries[static_cast<std::size_t>(index)];
                if (on_selection_change) on_selection_change(entry.path);
                if (mode == FilePickerMode::SaveFile && !entry.is_directory && on_filename_change)
                    on_filename_change(entry.name);
            },
            .on_activate = activate,
            .key = "file_picker_items",
            .layout = {.width = std::max(12,props.width-6),.height = std::max(3,props.height)}}));
        children.emplace_back(Typography({
            .text = !props.show_selection_info ? "" : selected_index >= 0 ?
                std::string(selected_directory ? "Folder: " : "File: ") + selected_path.filename().string() :
                (mode == FilePickerMode::Directory ? "Current folder can be selected" : "Select a file to continue"),
            .key = "file_picker_selection",
            .color = selected_index >= 0 ? ColorVariant::Primary : ColorVariant::Default}));
        if (mode == FilePickerMode::SaveFile)
        {
            children.emplace_back(TextField({.value = props.filename,
                .placeholder = "File name (e.g. report.md)",.key = "file_picker_filename",
                .layout = {.width = std::max(12,props.width-6)},
                .on_change = props.on_filename_change}));
            if (props.on_allow_overwrite_change)
                children.emplace_back(Checkbox({.label = "Allow overwrite of existing file",
                    .key = "file_picker_allow_overwrite",.checked = props.allow_overwrite,
                    .on_change = props.on_allow_overwrite_change}));
            std::error_code exists_error;
            if (validFilename(props.filename) &&
                std::filesystem::exists(directory / props.filename,exists_error) &&
                !props.allow_overwrite)
                children.emplace_back(Typography({.text = "File exists; overwriting is disabled",
                    .key = "file_picker_overwrite_warning",.color = ColorVariant::Warning}));
        }
        children.emplace_back(Row({.children = {
            Button({.text = "Cancel",.key = "file_picker_cancel",.color = ColorVariant::Default,
                .on_click = on_close}),
            Button({.text = mode == FilePickerMode::Directory ? "Select directory" :
                mode == FilePickerMode::SaveFile ? "Save" : "Select file",
                .key = "file_picker_confirm",.variant = ButtonVariant::Contained,
                .disabled = !can_confirm,
                .on_click = [result,on_confirm] { if (on_confirm) on_confirm(result); }})
        },.layout = {.gap = 1}}));
        return Dialog({.open = props.open,.title = std::move(props.title),
            .key = std::move(props.key),.width = props.width,.on_close = on_close,
            .children = std::move(children),.show_actions = false});
    }
}; /* End of class FilePicker */

} /* End of namespace terion */

#endif /* TERION_COMPONENTS_FILE_PICKER_H */
