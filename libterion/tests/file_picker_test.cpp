/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <chrono>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <terion/terion.h>

using namespace terion;

namespace {

class TemporaryDirectory
{
public:
    TemporaryDirectory()
    {
        const auto suffix = std::chrono::steady_clock::now().time_since_epoch().count();
        path_ = std::filesystem::temp_directory_path() / ("terion_file_picker_" + std::to_string(suffix));
        std::filesystem::create_directory(path_);
    }

    ~TemporaryDirectory()
    {
        std::error_code error;
        std::filesystem::remove_all(path_,error);
    }

    [[nodiscard]] const std::filesystem::path& path() const { return path_; }

private:
    std::filesystem::path path_;
}; /* End of class TemporaryDirectory */

} /* End of anonymous namespace */

TEST(FilePicker, ListsDirectoriesFirstAndFiltersExtensions)
{
    const TemporaryDirectory temporary;
    std::filesystem::create_directory(temporary.path() / "folder");
    std::ofstream(temporary.path() / "a.cpp").put('a');
    std::ofstream(temporary.path() / "b.txt").put('b');
    std::ofstream(temporary.path() / ".hidden.cpp").put('h');
    const auto listing = FilePickerBrowser::list(temporary.path(),
        {.extensions = {"CPP"},.show_hidden = false});
    EXPECT_TRUE(listing.error.empty());
    ASSERT_EQ(listing.entries.size(),2U);
    EXPECT_TRUE(listing.entries[0].is_directory);
    EXPECT_EQ(listing.entries[0].name,"folder");
    EXPECT_EQ(listing.entries[1].name,"a.cpp");
}

TEST(FilePicker, FiltersByNameAndCanShowHiddenEntries)
{
    const TemporaryDirectory temporary;
    std::ofstream(temporary.path() / ".secret.txt").put('s');
    std::ofstream(temporary.path() / "normal.txt").put('n');
    const auto listing = FilePickerBrowser::list(temporary.path(),
        {.search = "SECRET",.show_hidden = true});
    ASSERT_EQ(listing.entries.size(),1U);
    EXPECT_EQ(listing.entries[0].name,".secret.txt");
}

TEST(FilePicker, ReportsMissingDirectory)
{
    const TemporaryDirectory temporary;
    const auto listing = FilePickerBrowser::list(temporary.path() / "missing");
    EXPECT_FALSE(listing.error.empty());
    EXPECT_TRUE(listing.entries.empty());
}

TEST(FilePicker, BrowsesAndConfirmsSelectedFile)
{
    const TemporaryDirectory temporary;
    const auto file = temporary.path() / "a.txt";
    std::ofstream(file).put('a');
    std::filesystem::path selection;
    std::filesystem::path confirmed;
    const FilePicker picker({.open = true,.directory = temporary.path(),
        .selected_path = file,.icon_style = FilePickerIconStyle::Unicode,.on_selection_change = [&](std::filesystem::path value) {
            selection = std::move(value);
        },.on_confirm = [&](std::filesystem::path value) { confirmed = std::move(value); }});
    const auto& dialog = picker.props();
    EXPECT_EQ(dialog.type,Component::Type::Dialog);
    EXPECT_TRUE(dialog.dialog_open);
    ASSERT_EQ(dialog.children.size(),7U);
    const auto& list = dialog.children[4].props();
    EXPECT_EQ(list.type,Component::Type::VirtualList);
    ASSERT_EQ(list.virtual_item_count,1);
    EXPECT_EQ(list.virtual_item_text(0),"  📄    a.txt");
    EXPECT_EQ(list.virtual_row_height,2);
    ASSERT_TRUE(static_cast<bool>(list.virtual_on_select));
    list.virtual_on_select(0);
    EXPECT_EQ(selection,file);
    const auto& buttons = dialog.children[6].props().children;
    ASSERT_EQ(buttons.size(),2U);
    ASSERT_TRUE(static_cast<bool>(buttons[1].props().on_click));
    EXPECT_FALSE(buttons[1].props().disabled);
    buttons[1].props().on_click();
    EXPECT_EQ(confirmed,file);
}

TEST(FilePicker, DirectoryModeConfirmsCurrentDirectory)
{
    const TemporaryDirectory temporary;
    std::filesystem::path confirmed;
    const FilePicker picker({.open = true,.mode = FilePickerMode::Directory,
        .directory = temporary.path(),.on_confirm = [&](std::filesystem::path value) {
            confirmed = std::move(value);
        }});
    const auto& button = picker.props().children.back().props().children[1].props();
    EXPECT_FALSE(button.disabled);
    button.on_click();
    EXPECT_EQ(confirmed,temporary.path());
}

TEST(FilePicker, SingleClickSelectsDirectoryWithoutNavigating)
{
    const TemporaryDirectory temporary;
    const auto folder = temporary.path() / "folder";
    std::filesystem::create_directory(folder);
    std::filesystem::path selection;
    std::filesystem::path destination;
    const FilePicker picker({.open = true,.mode = FilePickerMode::Directory,
        .directory = temporary.path(),
        .on_directory_change = [&](std::filesystem::path value) { destination = std::move(value); },
        .on_selection_change = [&](std::filesystem::path value) { selection = std::move(value); }});
    const auto& list = picker.props().children[4].props();
    ASSERT_TRUE(static_cast<bool>(list.virtual_on_select));
    ASSERT_TRUE(static_cast<bool>(list.virtual_on_activate));
    list.virtual_on_select(0);
    EXPECT_EQ(selection,folder);
    EXPECT_TRUE(destination.empty());
    list.virtual_on_activate(0);
    EXPECT_EQ(destination,folder);
}

TEST(FilePicker, DialogMouseSelectsVirtualListRows)
{
    const TemporaryDirectory temporary;
    const auto file = temporary.path() / "click.txt";
    std::ofstream(file).put('a');
    std::filesystem::path selected;
    Reconciler tree;
    tree.update(FilePicker({.open = true,.directory = temporary.path(),
        .on_selection_change = [&](std::filesystem::path value) { selected = std::move(value); }}));
    ASSERT_NE(tree.root(),nullptr);
    auto* dialog = tree.root();
    const Size screen{.width = 90,.height = 30};
    dialog->layoutDialogChildren(screen);
    auto* list = dialog->children()[4].get();
    const Rect bounds = list->bounds();
    ASSERT_GT(bounds.width,0);
    ASSERT_GT(bounds.height,0);
    const Point point{.x = bounds.x+1,.y = bounds.y};
    ASSERT_TRUE(dialog->handleDialogMouse({.position = point,
        .button = MouseButton::Left,.action = MouseAction::Press},screen));
    ASSERT_TRUE(dialog->handleDialogMouse({.position = point,
        .button = MouseButton::Left,.action = MouseAction::Release},screen));
    EXPECT_EQ(selected,file);
}

TEST(FilePicker, ScrollWheelTargetsListInsideModal)
{
    const TemporaryDirectory temporary;
    for (int index = 0; index < 50; ++index)
        std::ofstream(temporary.path() / ("entry_" + std::to_string(index) + ".txt")).put('x');
    Reconciler tree;
    tree.update(FilePicker({.open = true,.directory = temporary.path()}));
    auto* dialog = tree.root();
    const Size screen{.width = 90,.height = 32};
    dialog->layoutDialogChildren(screen);
    auto* list = dialog->children()[4].get();
    ASSERT_NE(list,nullptr);
    const Rect bounds = list->bounds();
    ASSERT_TRUE(dialog->handleDialogMouse({.position = {.x = bounds.x+1,.y = bounds.y+1},
        .action = MouseAction::Scroll,.scroll_y = -2},screen));
    EXPECT_GT(list->virtual_list_offset(),0);
}

TEST(FilePicker, ShowsUnicodeIconsForDirectoriesAndFiles)
{
    const TemporaryDirectory temporary;
    std::filesystem::create_directory(temporary.path() / "folder");
    std::ofstream(temporary.path() / "example.txt").put('x');
    const FilePicker picker({.open = true,.directory = temporary.path(),
        .icon_style = FilePickerIconStyle::Unicode});
    const auto& list = picker.props().children[4].props();
    ASSERT_EQ(list.virtual_item_count,2);
    EXPECT_EQ(list.virtual_item_text(0),"📁  folder");
    EXPECT_EQ(list.virtual_item_text(1),"📄  example.txt");
}

TEST(FilePicker, SupportsAsciiAndIconFreeFallbacks)
{
    const TemporaryDirectory temporary;
    std::filesystem::create_directory(temporary.path() / "folder");
    std::ofstream(temporary.path() / "example.txt").put('x');
    const FilePicker ascii_picker({.open = true,.directory = temporary.path(),
        .icon_style = FilePickerIconStyle::Ascii});
    const auto& ascii_list = ascii_picker.props().children[4].props();
    EXPECT_EQ(ascii_list.virtual_item_text(0),"[DIR] folder");
    EXPECT_EQ(ascii_list.virtual_item_text(1),"[FILE] example.txt");

    const FilePicker plain_picker({.open = true,.directory = temporary.path(),
        .icon_style = FilePickerIconStyle::None});
    const auto& plain_list = plain_picker.props().children[4].props();
    EXPECT_EQ(plain_list.virtual_item_text(0),"folder");
    EXPECT_EQ(plain_list.virtual_item_text(1),"example.txt");
}

TEST(FilePicker, SymbolsUseStyledTextAndHighlightDirectories)
{
    const TemporaryDirectory temporary;
    std::filesystem::create_directory(temporary.path() / "folder");
    std::ofstream(temporary.path() / "example.txt").put('x');
    const FilePicker picker({.open = true,.directory = temporary.path()});
    const auto& list = picker.props().children[4].props();
    ASSERT_TRUE(static_cast<bool>(list.virtual_item_styled));
    ASSERT_EQ(list.virtual_item_count,2);
    EXPECT_EQ(list.virtual_item_text(0),"▸  folder");
    EXPECT_EQ(list.virtual_item_text(1),"·  example.txt");
    const auto folder = list.virtual_item_styled(0);
    const auto file = list.virtual_item_styled(1);
    ASSERT_EQ(folder.size(),2U);
    ASSERT_EQ(file.size(),2U);
    EXPECT_EQ(folder[0].color,ColorVariant::Info);
    EXPECT_TRUE(folder[1].bold);
    EXPECT_EQ(folder[1].text,"folder/");
    EXPECT_FALSE(file[1].bold);
    EXPECT_EQ(file[1].text,"example.txt");
}

TEST(FilePicker, BreadcrumbAndSelectionInformationCanBeHidden)
{
    const TemporaryDirectory temporary;
    const FilePicker picker({.open = true,.directory = temporary.path(),
        .show_breadcrumbs = false,.show_entry_count = false,.show_selection_info = false});
    const auto& children = picker.props().children;
    ASSERT_EQ(children.size(),7U);
    EXPECT_EQ(children[0].props().text,temporary.path().string());
    EXPECT_EQ(children[3].props().text,"Enter: open  |  Double-click: open");
    EXPECT_TRUE(children[5].props().text.empty());
}
