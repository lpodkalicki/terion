/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#ifndef TERION_COMPONENTS_FILE_PICKER_H
#define TERION_COMPONENTS_FILE_PICKER_H

#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <terion/components/breadcrumbs.h>
#include <terion/components/file_picker_browser.h>
#include <terion/components/file_picker_async.h>

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
        // Optional non-blocking filesystem provider. Without it, the picker
        // lists directories synchronously (useful for small, local paths).
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
        const std::filesystem::path& path,int width);

    [[nodiscard]] static bool validFilename(const std::string& name);

    static Component create(Props props);
}; /* End of class FilePicker */

} /* End of namespace terion */

#endif /* TERION_COMPONENTS_FILE_PICKER_H */
