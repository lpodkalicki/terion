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
#include <terion/components/column.h>
#include <terion/components/dialog.h>
#include <terion/components/row.h>
#include <terion/components/text_field.h>
#include <terion/components/typography.h>
#include <terion/components/virtual_list.h>

namespace terion {

enum class FilePickerMode
{
    File,
    Directory
}; /* End of enum FilePickerMode */

// Unicode icons are intended for UTF-8 terminals. Use Ascii if emoji widths are
// rendered inconsistently by the terminal or font; None shows names only.
enum class FilePickerIconStyle
{
    Unicode,
    Ascii,
    None,
    Symbols
}; /* End of enum FilePickerIconStyle */

struct FilePickerEntry
{
    std::filesystem::path path;
    std::string name;
    bool is_directory = false;
}; /* End of struct FilePickerEntry */

struct FilePickerListing
{
    std::filesystem::path directory;
    std::vector<FilePickerEntry> entries;
    std::string error;
}; /* End of struct FilePickerListing */

// Stateless filesystem enumeration. Failure is reported via FilePickerListing::error,
// not an exception. Directories remain navigable even when extension filters are set.
class FilePickerBrowser
{
public:
    struct Options
    {
        std::vector<std::string> extensions = {};
        std::string search = {};
        bool show_hidden = false;
    }; /* End of struct Options */

    [[nodiscard]] static FilePickerListing list(const std::filesystem::path& directory)
    {
        return list(directory,Options{});
    }

    [[nodiscard]] static FilePickerListing list(const std::filesystem::path& directory,
                                                const Options& options)
    {
        FilePickerListing listing;
        listing.directory = directory;
        std::error_code error;
        const auto status = std::filesystem::status(directory,error);
        if (error || !std::filesystem::is_directory(status))
        {
            listing.error = error ? error.message() : "Not a directory";
            return listing;
        }
        std::filesystem::directory_iterator iterator(
            directory,std::filesystem::directory_options::skip_permission_denied,error);
        if (error)
        {
            listing.error = error.message();
            return listing;
        }
        const std::filesystem::directory_iterator end;
        while (iterator != end)
        {
            const auto& path = iterator->path();
            const auto name = path.filename().string();
            std::error_code entry_error;
            const bool is_directory = iterator->is_directory(entry_error);
            if (!entry_error && (options.show_hidden || name.empty() || name.front() != '.') &&
                (options.search.empty() || containsInsensitive(name,options.search)) &&
                (is_directory || hasExtension(path,options.extensions)))
                listing.entries.push_back({.path = path,.name = name,.is_directory = is_directory});
            iterator.increment(error);
            if (error)
            {
                listing.error = error.message();
                break;
            }
        }
        std::sort(listing.entries.begin(),listing.entries.end(),
            [](const FilePickerEntry& lhs,const FilePickerEntry& rhs) {
                if (lhs.is_directory != rhs.is_directory) return lhs.is_directory;
                return lowercase(lhs.name) < lowercase(rhs.name);
            });
        return listing;
    }

private:
    [[nodiscard]] static std::string lowercase(std::string_view value)
    {
        std::string result(value);
        std::transform(result.begin(),result.end(),result.begin(),[](unsigned char character) {
            return static_cast<char>(std::tolower(character));
        });
        return result;
    }

    [[nodiscard]] static bool containsInsensitive(std::string_view haystack,std::string_view needle)
    {
        return lowercase(haystack).find(lowercase(needle)) != std::string::npos;
    }

    [[nodiscard]] static bool hasExtension(const std::filesystem::path& path,
                                           const std::vector<std::string>& extensions)
    {
        if (extensions.empty()) return true;
        const auto extension = lowercase(path.extension().string());
        for (const auto& candidate : extensions)
        {
            const auto normalized = lowercase(candidate);
            if (extension == (normalized.empty() || normalized.front() == '.' ? normalized : "." + normalized))
                return true;
        }
        return false;
    }
}; /* End of class FilePickerBrowser */

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
    }; /* End of struct Props */

    explicit FilePicker(Props props) : Component(create(std::move(props))) {}

private:
    [[nodiscard]] static std::string breadcrumbText(const std::filesystem::path& path, int width)
    {
        std::vector<std::string> segments;
        for (const auto& part : path)
        {
            const std::string segment = part.string();
            if (!segment.empty() && segment != "/") segments.push_back(segment);
        }
        const int budget = std::max(14,width-12);
        std::string result;
        bool shortened = false;
        for (auto iter = segments.rbegin(); iter != segments.rend(); ++iter)
        {
            const std::string candidate = *iter + (result.empty() ? "" : " / " + result);
            if (static_cast<int>(candidate.size()) > budget && !result.empty())
            {
                shortened = true;
                break;
            }
            result = candidate;
        }
        if (result.empty()) result = path.root_path().empty() ? "." : "/";
        else if (shortened) result = "... / " + result;
        else if (path.is_absolute()) result = "/ " + result;
        return result;
    }

    static Component create(Props props)
    {
        std::error_code error;
        auto directory = props.directory;
        if (directory.empty()) directory = std::filesystem::current_path(error);
        if (error) directory.clear();

        auto listing = std::make_shared<FilePickerListing>();
        if (props.open && !directory.empty())
            *listing = FilePickerBrowser::list(directory,
                {.extensions = props.extensions,.search = props.search,.show_hidden = props.show_hidden});
        else if (props.open) listing->error = "Cannot determine current directory";

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

        const auto activate = [listing,navigate,on_confirm,mode](const int index) {
            if (index < 0 || static_cast<std::size_t>(index) >= listing->entries.size()) return;
            const auto& entry = listing->entries[static_cast<std::size_t>(index)];
            if (entry.is_directory) navigate(entry.path);
            else if (mode == FilePickerMode::File && on_confirm) on_confirm(entry.path);
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

        const bool selected_directory = selected_index >= 0 &&
            listing->entries[static_cast<std::size_t>(selected_index)].is_directory;
        std::vector<Component> children;
        children.reserve(8);
        children.emplace_back(Typography({
            .text = props.show_breadcrumbs ? breadcrumbText(directory,props.width) : directory.string(),
            .key = "file_picker_location",.color = ColorVariant::Info,
            .style = {.bold = true}}));
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
                }})
        },.layout = {.gap = 1}}));
        children.emplace_back(TextField({.value = props.search,.placeholder = "Search filenames...",
            .key = "file_picker_search",.layout = {.width = std::max(12,props.width-6)},
            .on_change = on_search_change}));
        if (!listing->error.empty())
            children.emplace_back(Typography({.text = "Filesystem error: " + listing->error,
                .key = "file_picker_error",.color = ColorVariant::Error}));
        else
            children.emplace_back(Typography({
                .text = (props.show_entry_count ? std::to_string(listing->entries.size()) + " entries  |  " : "") +
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
            .on_select = [listing,on_selection_change](const int index) {
                if (index < 0 || static_cast<std::size_t>(index) >= listing->entries.size()) return;
                if (on_selection_change) on_selection_change(listing->entries[static_cast<std::size_t>(index)].path);
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
        children.emplace_back(Row({.children = {
            Button({.text = "Cancel",.key = "file_picker_cancel",.color = ColorVariant::Default,
                .on_click = on_close}),
            Button({.text = mode == FilePickerMode::Directory ? "Select directory" : "Select file",
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
