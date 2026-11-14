/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#ifndef TERION_COMPONENTS_FILE_PICKER_BROWSER_H
#define TERION_COMPONENTS_FILE_PICKER_BROWSER_H

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace terion {

enum class FilePickerMode
{
    File,
    Directory,
    SaveFile
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

} /* End of namespace terion */

#endif /* TERION_COMPONENTS_FILE_PICKER_BROWSER_H */
