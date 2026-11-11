/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <terion/terion.h>
#include <gtest/gtest.h>

using namespace terion;

TEST(Select, SupportsScrollbarDraggingAndPopupPlacement)
{
    std::vector<SelectOption> long_options;
    for (int i = 0; i < 12; ++i)
        long_options.push_back({.label = std::to_string(i),.value = std::to_string(i)});
    Reconciler long_select;
    long_select.update(Select({.options = long_options,.value = "7",.layout = {.width = 15},
        .on_change = [](std::string) {}}));
    const Size menu_screen {.width = 24,.height = 8};
    long_select.root()->layout(Constraints{.maximum = menu_screen});
    EXPECT_TRUE(long_select.root()->handleSelectKey(Event{.key = Key::Enter},menu_screen));
    const Rect long_popup = long_select.root()->selectPopupRect(menu_screen);
    EXPECT_TRUE(long_popup.y == 3 && long_popup.height == 5);
    EXPECT_EQ(long_select.root()->select_scrollbar_track(menu_screen).height,5);
    ScreenBuffer long_buffer(menu_screen);
    Painter long_painter(long_buffer);
    long_select.root()->paintOverlay(long_painter,menu_screen,Theme::dark());
    EXPECT_EQ(long_buffer.at(long_popup.x+1,long_popup.y).grapheme,"5");
    const Rect long_thumb = long_select.root()->select_scrollbar_thumb(menu_screen);
    const int scrollbar_x = long_select.root()->select_scrollbar_track(menu_screen).x;
    EXPECT_TRUE(long_select.root()->handleSelectMouse(MouseEvent{.position = {.x = scrollbar_x,.y = long_thumb.y},
        .button = MouseButton::Left,.action = MouseAction::Press},menu_screen));
    EXPECT_TRUE(long_select.root()->selectDragging());
    EXPECT_TRUE(long_select.root()->handleSelectMouse(MouseEvent{.position = {.x = scrollbar_x,.y = 100},
        .button = MouseButton::Left,.action = MouseAction::Move},menu_screen));
    EXPECT_TRUE(long_select.root()->handleSelectMouse(MouseEvent{.position = {.x = scrollbar_x,.y = 100},
        .button = MouseButton::Left,.action = MouseAction::Release},menu_screen));
    EXPECT_TRUE(!long_select.root()->selectDragging());
    long_buffer.clear();
    long_select.root()->paintOverlay(long_painter,menu_screen,Theme::dark());
    EXPECT_EQ(long_buffer.at(long_popup.x+1,long_popup.y).grapheme,"7");
    long_select.root()->closeSelect();
    long_select.root()->layout(Constraints{.maximum = {.width = 24,.height = 9}},{.x = 0,.y = 6});
    EXPECT_TRUE(long_select.root()->handleSelectKey(Event{.key = Key::Enter},{.width = 24,.height = 9}));
    EXPECT_EQ(long_select.root()->selectPopupRect({.width = 24,.height = 9}).y,0);
}

TEST(Select, HandlesKeyboardMouseAndOverlayDismissal)
{
    State<std::string> chosen("a");
    Application select_app(std::make_unique<HeadlessTerminalBackend>(Size{.width = 30,.height = 12}));
    select_app.render([&] {
        return Column({.children = {
            Select({.options = {{.label = "Alpha",.value = "a"},
                {.label = "Beta",.value = "b"},
                {.label = "Disabled",.value = "c",.disabled = true}},
                .value = chosen.get(),.key = "choice",.layout = {.width = 15},
                .on_change = [&](std::string value) { chosen.set(std::move(value)); }}),
            Select({.options = {{.label = "One",.value = "1"},{.label = "Two",.value = "2"}},
                .value = "1",.key = "second",.layout = {.width = 15},
                .on_change = [](std::string) {}})
        }});
    });
    select_app.renderOnce();
    auto* select_node = select_app.reconciler().root()->children()[0].get();
    EXPECT_TRUE(!select_node->selectOpen());
    EXPECT_TRUE(select_app.dispatchEvent(Event{.key = Key::Enter}));
    EXPECT_TRUE(select_node->selectOpen());
    EXPECT_TRUE(select_app.dispatchEvent(Event{.key = Key::Down}));
    EXPECT_TRUE(select_app.dispatchEvent(Event{.key = Key::Enter}));
    EXPECT_EQ(chosen.get(),"b");
    EXPECT_TRUE(!select_node->selectOpen());
    select_app.renderOnce();
    select_node = select_app.reconciler().root()->children()[0].get();
    EXPECT_TRUE(select_app.dispatchEvent(Event{.key = Key::Enter}));
    const Rect popup = select_node->selectPopupRect(select_app.backend().size());
    EXPECT_EQ(popup.height,3);
    ScreenBuffer select_buffer(select_app.backend().size());
    Painter select_painter(select_buffer);
    select_painter.text(popup.x+2,popup.y,"Text behind the popup");
    select_painter.text(popup.x+3,popup.y+1,"Another underlying label");
    select_app.reconciler().root()->paintOverlays(select_painter,select_buffer.size(),Theme::dark());
    EXPECT_EQ(select_buffer.at(popup.x+1,popup.y).grapheme,"A");
    EXPECT_EQ(select_buffer.at(popup.x+8,popup.y).grapheme," ");
    EXPECT_EQ(select_buffer.at(popup.x+8,popup.y+1).grapheme," ");
    EXPECT_TRUE(select_app.dispatchEvent(Event{.key = Key::Mouse,.mouse = {
        .position = {.x = popup.x+1,.y = popup.y},.button = MouseButton::Left,.action = MouseAction::Press}}));
    EXPECT_EQ(chosen.get(),"a");
    EXPECT_TRUE(!select_node->selectOpen());
    select_app.renderOnce();
    EXPECT_TRUE(select_app.dispatchEvent(Event{.key = Key::Enter}));
    EXPECT_TRUE(select_app.dispatchEvent(Event{.key = Key::Escape}));
    EXPECT_TRUE(!select_app.reconciler().root()->children()[0]->selectOpen());

    OverlayManager overlays;
    Reconciler overlay_tree;
    overlay_tree.update(Select({.options = {{.label = "Overlay",.value = "o"}},
        .value = "o",.on_change = [](std::string) {}}));
    overlays.collect(overlay_tree.root());
    EXPECT_TRUE(overlay_tree.root()->handleSelectKey(Event{.key = Key::Enter}));
    overlays.dismissExcept(nullptr);
    EXPECT_TRUE(!overlay_tree.root()->selectOpen());
    EXPECT_TRUE(select_app.dispatchEvent(Event{.key = Key::Enter}));
    EXPECT_TRUE(select_app.reconciler().root()->children()[0]->selectOpen());
    select_app.focus().focus(std::to_string(select_app.reconciler().root()->children()[1]->id()));
    EXPECT_TRUE(select_app.dispatchEvent(Event{.key = Key::Enter}));
    EXPECT_TRUE(!select_app.reconciler().root()->children()[0]->selectOpen());
    EXPECT_TRUE(select_app.reconciler().root()->children()[1]->selectOpen());
    EXPECT_TRUE(select_app.dispatchEvent(Event{.key = Key::Enter}));
    EXPECT_EQ(select_app.dispatchEvent(Event{.key = Key::Mouse,.mouse = {
        .position = {.x = 25,.y = 10},.button = MouseButton::Left,.action = MouseAction::Press}}),false);
    EXPECT_TRUE(!select_app.reconciler().root()->children()[0]->selectOpen());

}
