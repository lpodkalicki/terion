/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <chrono>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <thread>
#include <gtest/gtest.h>
#include <terion/terion.h>

using namespace terion;

namespace {

class TempFiles
{
public:
    TempFiles()
    {
        static std::size_t serial = 0;
        path_ = std::filesystem::temp_directory_path() /
            ("terion_file_picker_advanced_" + std::to_string(
                std::chrono::steady_clock::now().time_since_epoch().count()) +
                "_" + std::to_string(++serial));
        std::filesystem::create_directories(path_);
    }

    ~TempFiles()
    {
        std::error_code error;
        std::filesystem::remove_all(path_,error);
    }

    const std::filesystem::path& path() const { return path_; }

    void write(const std::string& name) const { std::ofstream(path_ / name).put('x'); }

private:
    std::filesystem::path path_;
};

FilePickerAsyncBrowser::Snapshot awaitListing(FilePickerAsyncBrowser& source,
    const std::filesystem::path& path,const FilePickerBrowser::Options& options = {})
{
    const auto deadline = std::chrono::steady_clock::now()+std::chrono::seconds(3);
    for (;;) {
        auto snapshot = source.request(path,options);
        if (snapshot.ready && !snapshot.loading) return snapshot;
        if (std::chrono::steady_clock::now()>deadline) return snapshot;
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
}

const Component::Props& confirmButton(const FilePicker& picker)
{
    const auto& footer = picker.props().children.back().props();
    return footer.children.back().props();
}

} /* End of anonymous namespace */

TEST(FilePickerAdvanced, AsynchronousListingCachesUntilInvalidated)
{
    const TempFiles temporary;
    temporary.write("first.txt");
    FilePickerAsyncBrowser source({.cache_capacity = 2,.cache_ttl = std::chrono::minutes(5)});
    const auto initial = source.request(temporary.path());
    EXPECT_TRUE(initial.loading);
    const auto loaded = awaitListing(source,temporary.path());
    ASSERT_TRUE(loaded.ready);
    ASSERT_NE(loaded.listing,nullptr);
    ASSERT_EQ(loaded.listing->entries.size(),1U);
    EXPECT_EQ(loaded.listing->entries[0].name,"first.txt");
    EXPECT_EQ(source.cachedCount(),1U);
    EXPECT_TRUE(source.consumeUpdates());

    temporary.write("second.txt");
    const auto cached = source.request(temporary.path());
    ASSERT_TRUE(cached.ready);
    EXPECT_FALSE(cached.loading);
    ASSERT_EQ(cached.listing,loaded.listing);  // immutable shared snapshot: no large copies
    ASSERT_EQ(cached.listing->entries.size(),1U);

    source.invalidate(temporary.path());
    const auto refreshed = awaitListing(source,temporary.path());
    ASSERT_TRUE(refreshed.ready);
    ASSERT_NE(refreshed.listing,nullptr);
    EXPECT_EQ(refreshed.listing->entries.size(),2U);
}

TEST(FilePickerAdvanced, AsyncSearchAndDirectoryChangesNeverExposeWrongListing)
{
    const TempFiles temporary;
    temporary.write("alpha.txt");
    temporary.write("beta.txt");
    FilePickerAsyncBrowser source;
    const FilePickerBrowser::Options filter{.search = "BETA"};
    (void)source.request(temporary.path(),{.search = "ALPHA"});
    const auto result = awaitListing(source,temporary.path(),filter);
    ASSERT_TRUE(result.ready);
    ASSERT_NE(result.listing,nullptr);
    ASSERT_EQ(result.listing->entries.size(),1U);
    EXPECT_EQ(result.listing->entries[0].name,"beta.txt");
    EXPECT_EQ(result.listing->directory,temporary.path());
}

TEST(FilePickerAdvanced, CacheCapacityAndRefreshAreBounded)
{
    const TempFiles temporary;
    const auto second = temporary.path() / "second";
    std::filesystem::create_directory(second);
    FilePickerAsyncBrowser source({.cache_capacity = 1,.cache_ttl = std::chrono::minutes(5)});
    ASSERT_TRUE(awaitListing(source,temporary.path()).ready);
    ASSERT_TRUE(awaitListing(source,second).ready);
    EXPECT_LE(source.cachedCount(),1U);
    source.invalidateAll();
    EXPECT_EQ(source.cachedCount(),0U);
}

TEST(FilePickerAdvanced, BreadcrumbsNavigateToParent)
{
    const TempFiles temporary;
    const auto child = temporary.path() / "child";
    std::filesystem::create_directory(child);
    std::filesystem::path destination;
    const FilePicker picker({.open = true,.directory = child,.width = 120,
        .on_directory_change = [&](std::filesystem::path value) {
            destination = std::move(value);
        }});
    const auto& crumbs = picker.props().children[0].props();
    ASSERT_EQ(crumbs.type,Component::Type::Row);
    bool found_parent = false;
    for (const auto& component : crumbs.children)
    {
        const auto& props = component.props();
        if (props.text == temporary.path().filename().string() && props.on_click)
        {
            props.on_click();
            found_parent = true;
            break;
        }
    }
    EXPECT_TRUE(found_parent);
    EXPECT_EQ(destination,temporary.path());
}

TEST(FilePickerAdvanced, EditablePathNavigatesRelativeDirectory)
{
    const TempFiles temporary;
    std::filesystem::create_directory(temporary.path() / "sub");
    std::filesystem::path destination;
    const FilePicker picker({.open = true,.directory = temporary.path(),
        .on_directory_change = [&](std::filesystem::path value) { destination = std::move(value); },
        .show_path_input = true,.path_input = "sub"});
    const auto& row = picker.props().children[1].props();
    ASSERT_EQ(row.type,Component::Type::Row);
    ASSERT_EQ(row.children.size(),2U);
    ASSERT_TRUE(row.children[1].props().on_click);
    row.children[1].props().on_click();
    EXPECT_EQ(destination,temporary.path() / "sub");
}

TEST(FilePickerAdvanced, SaveModePreventsTraversalAndAccidentalOverwrite)
{
    const TempFiles temporary;
    temporary.write("existing.md");
    std::filesystem::path confirmed;
    for (const auto& name : {"", "..", "../escape.md", "/tmp/escape.md", "sub/name.md", "existing.md"})
    {
        const FilePicker picker({.open = true,.mode = FilePickerMode::SaveFile,
            .directory = temporary.path(),.on_confirm = [&](std::filesystem::path value) {
                confirmed = std::move(value);
            },.filename = name});
        EXPECT_TRUE(confirmButton(picker).disabled) << name;
    }
    const FilePicker new_file({.open = true,.mode = FilePickerMode::SaveFile,
        .directory = temporary.path(),.on_confirm = [&](std::filesystem::path value) {
            confirmed = std::move(value);
        },.filename = "new.md"});
    ASSERT_FALSE(confirmButton(new_file).disabled);
    confirmButton(new_file).on_click();
    EXPECT_EQ(confirmed,temporary.path() / "new.md");
    EXPECT_FALSE(std::filesystem::exists(confirmed)); // picker does NOT write

    const FilePicker overwrite({.open = true,.mode = FilePickerMode::SaveFile,
        .directory = temporary.path(),.on_confirm = [&](std::filesystem::path value) {
            confirmed = std::move(value);
        },.filename = "existing.md",.allow_overwrite = true});
    ASSERT_FALSE(confirmButton(overwrite).disabled);
    confirmButton(overwrite).on_click();
    EXPECT_EQ(confirmed,temporary.path() / "existing.md");
}

TEST(FilePickerAdvanced, SelectingExistingFileSetsSaveName)
{
    const TempFiles temporary;
    temporary.write("existing.md");
    std::string filename;
    const FilePicker picker({.open = true,.mode = FilePickerMode::SaveFile,
        .directory = temporary.path(),.on_filename_change = [&](std::string value) {
            filename = std::move(value);
        }});
    const auto& list = picker.props().children[4].props();
    ASSERT_EQ(list.virtual_item_count,1);
    list.virtual_on_select(0);
    EXPECT_EQ(filename,"existing.md");
}
