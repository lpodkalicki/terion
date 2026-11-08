/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <terion/terion.h>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <iostream>

using namespace terion;

int main()
{
    State<bool> show_dialog(true);
    int confirmations = 0;
    int background_clicks = 0;
    Application dialog_app(std::make_unique<HeadlessTerminalBackend>(Size{.width = 50,.height = 16}));
    dialog_app.render([&] {
        return Column({.children = {
            Button({.text = "Background",.key = "background",.onClick = [&] { ++background_clicks; }}),
            Dialog({.open = show_dialog.get(),.title = "Confirm",.message = "Proceed?",.key = "modal",
                .onConfirm = [&] { ++confirmations; show_dialog.set(false); },
                .onClose = [&] { show_dialog.set(false); }})
        }});
    });
    dialog_app.renderOnce();
    const auto* dialog_node = dialog_app.reconciler().root()->children()[1].get();
    const Rect dialog_rect = dialog_node->dialogRect(dialog_app.backend().size());
    assert(dialog_rect.width == 48 && dialog_rect.height == 9);
    ScreenBuffer dim_buffer({.width = 10,.height = 2});
    Painter dim_painter(dim_buffer);
    dim_painter.text(0,0,"VISIBLE");
    const Color before_dim = dim_buffer.at(0,0).style.foreground;
    dim_painter.dim({.width = 10,.height = 2},Theme::dark().palette.background);
    assert(dim_buffer.at(0,0).grapheme == "V");
    assert(dim_buffer.at(0,0).style.foreground != before_dim);
    Reconciler dialog_layout;
    dialog_layout.update(Column({.children = {
        Typography({.text = "Label"}),
        Dialog({.open = true,.title = "Hidden from layout"})
    },.layout = {.gap = 3}}));
    assert(dialog_layout.root()->measure(Constraints{.maximum = {.width = 40,.height = 20}}).height == 1);
    assert(dialog_app.dispatchEvent(Event{.key = Key::Mouse,.mouse = {
        .position = {.x = 1,.y = 1},.button = MouseButton::Left,.action = MouseAction::Press}}));
    assert(background_clicks == 0 && !show_dialog.get());
    dialog_app.renderOnce();
    show_dialog.set(true);
    dialog_app.renderOnce();
    assert(dialog_app.dispatchEvent(Event{.key = Key::Tab}));
    assert(dialog_app.dispatchEvent(Event{.key = Key::Enter}));
    assert(confirmations == 1 && !show_dialog.get());
    show_dialog.set(true);
    dialog_app.renderOnce();
    assert(dialog_app.dispatchEvent(Event{.key = Key::Escape}));
    assert(!show_dialog.get());

    std::vector<SelectOption> long_options;
    for (int i = 0; i < 12; ++i)
        long_options.push_back({.label = std::to_string(i),.value = std::to_string(i)});
    Reconciler long_select;
    long_select.update(Select({.options = long_options,.value = "7",.layout = {.width = 15},
        .onChange = [](std::string) {}}));
    const Size menu_screen {.width = 24,.height = 8};
    long_select.root()->layout(Constraints{.maximum = menu_screen});
    assert(long_select.root()->handleSelectKey(Event{.key = Key::Enter},menu_screen));
    const Rect long_popup = long_select.root()->selectPopupRect(menu_screen);
    assert(long_popup.y == 3 && long_popup.height == 5);
    assert(long_select.root()->selectScrollbarTrack(menu_screen).height == 5);
    ScreenBuffer long_buffer(menu_screen);
    Painter long_painter(long_buffer);
    long_select.root()->paintOverlay(long_painter,menu_screen,Theme::dark());
    assert(long_buffer.at(long_popup.x+1,long_popup.y).grapheme == "5");
    const Rect long_thumb = long_select.root()->selectScrollbarThumb(menu_screen);
    const int scrollbar_x = long_select.root()->selectScrollbarTrack(menu_screen).x;
    assert(long_select.root()->handleSelectMouse(MouseEvent{.position = {.x = scrollbar_x,.y = long_thumb.y},
        .button = MouseButton::Left,.action = MouseAction::Press},menu_screen));
    assert(long_select.root()->selectDragging());
    assert(long_select.root()->handleSelectMouse(MouseEvent{.position = {.x = scrollbar_x,.y = 100},
        .button = MouseButton::Left,.action = MouseAction::Move},menu_screen));
    assert(long_select.root()->handleSelectMouse(MouseEvent{.position = {.x = scrollbar_x,.y = 100},
        .button = MouseButton::Left,.action = MouseAction::Release},menu_screen));
    assert(!long_select.root()->selectDragging());
    long_buffer.clear();
    long_select.root()->paintOverlay(long_painter,menu_screen,Theme::dark());
    assert(long_buffer.at(long_popup.x+1,long_popup.y).grapheme == "7");
    long_select.root()->closeSelect();
    long_select.root()->layout(Constraints{.maximum = {.width = 24,.height = 9}},{.x = 0,.y = 6});
    assert(long_select.root()->handleSelectKey(Event{.key = Key::Enter},{.width = 24,.height = 9}));
    assert(long_select.root()->selectPopupRect({.width = 24,.height = 9}).y == 0);

    State<std::string> chosen("a");
    Application select_app(std::make_unique<HeadlessTerminalBackend>(Size{.width = 30,.height = 12}));
    select_app.render([&] {
        return Column({.children = {
            Select({.options = {{.label = "Alpha",.value = "a"},
                {.label = "Beta",.value = "b"},
                {.label = "Disabled",.value = "c",.disabled = true}},
                .value = chosen.get(),.key = "choice",.layout = {.width = 15},
                .onChange = [&](std::string value) { chosen.set(std::move(value)); }}),
            Select({.options = {{.label = "One",.value = "1"},{.label = "Two",.value = "2"}},
                .value = "1",.key = "second",.layout = {.width = 15},
                .onChange = [](std::string) {}})
        }});
    });
    select_app.renderOnce();
    auto* select_node = select_app.reconciler().root()->children()[0].get();
    assert(!select_node->selectOpen());
    assert(select_app.dispatchEvent(Event{.key = Key::Enter}));
    assert(select_node->selectOpen());
    assert(select_app.dispatchEvent(Event{.key = Key::Down}));
    assert(select_app.dispatchEvent(Event{.key = Key::Enter}));
    assert(chosen.get() == "b");
    assert(!select_node->selectOpen());
    select_app.renderOnce();
    select_node = select_app.reconciler().root()->children()[0].get();
    assert(select_app.dispatchEvent(Event{.key = Key::Enter}));
    const Rect popup = select_node->selectPopupRect(select_app.backend().size());
    assert(popup.height == 3);
    ScreenBuffer select_buffer(select_app.backend().size());
    Painter select_painter(select_buffer);
    select_painter.text(popup.x+2,popup.y,"Text behind the popup");
    select_painter.text(popup.x+3,popup.y+1,"Another underlying label");
    select_app.reconciler().root()->paintOverlays(select_painter,select_buffer.size(),Theme::dark());
    assert(select_buffer.at(popup.x+1,popup.y).grapheme == "A");
    assert(select_buffer.at(popup.x+8,popup.y).grapheme == " ");
    assert(select_buffer.at(popup.x+8,popup.y+1).grapheme == " ");
    assert(select_app.dispatchEvent(Event{.key = Key::Mouse,.mouse = {
        .position = {.x = popup.x+1,.y = popup.y},.button = MouseButton::Left,.action = MouseAction::Press}}));
    assert(chosen.get() == "a");
    assert(!select_node->selectOpen());
    select_app.renderOnce();
    assert(select_app.dispatchEvent(Event{.key = Key::Enter}));
    assert(select_app.dispatchEvent(Event{.key = Key::Escape}));
    assert(!select_app.reconciler().root()->children()[0]->selectOpen());

    OverlayManager overlays;
    Reconciler overlay_tree;
    overlay_tree.update(Select({.options = {{.label = "Overlay",.value = "o"}},
        .value = "o",.onChange = [](std::string) {}}));
    overlays.collect(overlay_tree.root());
    assert(overlay_tree.root()->handleSelectKey(Event{.key = Key::Enter}));
    overlays.dismissExcept(nullptr);
    assert(!overlay_tree.root()->selectOpen());
    assert(select_app.dispatchEvent(Event{.key = Key::Enter}));
    assert(select_app.reconciler().root()->children()[0]->selectOpen());
    select_app.focus().focus(std::to_string(select_app.reconciler().root()->children()[1]->id()));
    assert(select_app.dispatchEvent(Event{.key = Key::Enter}));
    assert(!select_app.reconciler().root()->children()[0]->selectOpen());
    assert(select_app.reconciler().root()->children()[1]->selectOpen());
    assert(select_app.dispatchEvent(Event{.key = Key::Enter}));
    assert(select_app.dispatchEvent(Event{.key = Key::Mouse,.mouse = {
        .position = {.x = 25,.y = 10},.button = MouseButton::Left,.action = MouseAction::Press}}) == false);
    assert(!select_app.reconciler().root()->children()[0]->selectOpen());

    assert(!AnsiTerminalBackend::detectCapabilities("Apple_Terminal").mousePointerShape);
    assert(!AnsiTerminalBackend::detectCapabilities("vscode").mousePointerShape);
    assert(!AnsiTerminalBackend::detectCapabilities("iTerm.app").mousePointerShape);
    assert(AnsiTerminalBackend::detectCapabilities("ghostty").mousePointerShape);
    assert(AnsiTerminalBackend::detectCapabilities("kitty").mousePointerShape);
    assert(AnsiTerminalBackend::detectCapabilities("Apple_Terminal", "1").mousePointerShape);
    assert(!AnsiTerminalBackend::detectCapabilities("kitty", "0").mousePointerShape);
    auto pointer_backend = std::make_unique<HeadlessTerminalBackend>(Size{.width = 30,.height = 7});
    auto* pointer_output = pointer_backend.get();
    Application pointer_app(std::move(pointer_backend));
    pointer_app.render([] {
        return Column({.children = {
            Typography({.text = "Label"}),
            TextField({.value = "Edit",.key = "edit",.onChange = [](std::string) {},
                .size = ButtonSize::Compact}),
            Button({.text = "Click",.key = "click",.onClick = [] {},.size = ButtonSize::Compact}),
            Box({.children = {Typography({.text = "Box"})},.layout = {.width = 20,.height = 3}})
        }});
    });
    pointer_app.renderOnce();
    pointer_output->clearOutput();
    pointer_app.dispatchEvent(Event{.key = Key::Mouse,.mouse = {
        .position = {.x = 1,.y = 0},.action = MouseAction::Move}});
    assert(pointer_output->output().find("\033]22;text\033\\") != std::string::npos);
    pointer_output->clearOutput();
    pointer_app.dispatchEvent(Event{.key = Key::Mouse,.mouse = {
        .position = {.x = 15,.y = 0},.action = MouseAction::Move}});
    assert(pointer_output->output().find("\033]22;default\033\\") != std::string::npos);
    pointer_output->clearOutput();
    pointer_app.dispatchEvent(Event{.key = Key::Mouse,.mouse = {
        .position = {.x = 2,.y = 1},.action = MouseAction::Move}});
    assert(pointer_output->output().find("\033]22;text\033\\") != std::string::npos);
    pointer_output->clearOutput();
    pointer_app.dispatchEvent(Event{.key = Key::Mouse,.mouse = {
        .position = {.x = 2,.y = 2},.action = MouseAction::Move}});
    assert(pointer_output->output().find("\033]22;pointer\033\\") != std::string::npos);
    pointer_output->clearOutput();
    pointer_app.dispatchEvent(Event{.key = Key::Mouse,.mouse = {
        .position = {.x = 2,.y = 2},.action = MouseAction::Move}});
    assert(pointer_output->output().empty());
    pointer_app.dispatchEvent(Event{.key = Key::Mouse,.mouse = {
        .position = {.x = 15,.y = 4},.action = MouseAction::Move}});
    assert(pointer_output->output().find("\033]22;default\033\\") != std::string::npos);

    ScreenBuffer clipped_buffer({.width = 10,.height = 4});
    Painter scoped_painter(clipped_buffer);
    {
        Painter::ClipScope outer(scoped_painter,{.x = 1,.y = 1,.width = 7,.height = 2});
        scoped_painter.text(0,1,"ABCDEFGHIJ");
        assert(clipped_buffer.at(0,1).grapheme == " ");
        assert(clipped_buffer.at(1,1).grapheme == "B");
        assert(clipped_buffer.at(7,1).grapheme == "H");
        assert(clipped_buffer.at(8,1).grapheme == " ");
        {
            Painter::ClipScope inner(scoped_painter,{.x = 3,.y = 0,.width = 2,.height = 4});
            scoped_painter.fill({.width = 10,.height = 4},Cell{.grapheme = "X"});
        }
        assert(clipped_buffer.at(2,1).grapheme == "C");
        assert(clipped_buffer.at(3,1).grapheme == "X");
        assert(clipped_buffer.at(4,1).grapheme == "X");
        scoped_painter.put(8,1,"Z");
        assert(clipped_buffer.at(8,1).grapheme == " ");
    }
    scoped_painter.put(8,1,"Z");
    assert(clipped_buffer.at(8,1).grapheme == "Z");

    int reported_scroll = -1;
    Reconciler scrolling;
    scrolling.update(ScrollView({.children = {
        Typography({.text = "0"}),Typography({.text = "1"}),Typography({.text = "2"}),
        Typography({.text = "3"}),Typography({.text = "4"}),Typography({.text = "5"})
    },.layout = {.width = 5,.height = 3},.scrollOffset = 2,
        .onScroll = [&](int offset) { reported_scroll = offset; }}));
    scrolling.root()->layout(Constraints{.maximum = {.width = 5,.height = 3}});
    ScreenBuffer scroll_buffer({.width = 5,.height = 6});
    Painter scroll_painter(scroll_buffer);
    scrolling.root()->paint(scroll_painter);
    assert(scroll_buffer.at(0,0).grapheme == "2");
    assert(scroll_buffer.at(0,2).grapheme == "4");
    assert(scroll_buffer.at(0,3).grapheme == " ");
    assert(scroll_buffer.at(4,0).grapheme == "│");
    assert(scroll_buffer.at(4,1).grapheme == "█");
    assert(scrolling.root()->scrollbarTrack().x == 4);
    assert(scrolling.root()->scrollbarThumb().y == 1);
    ScreenBuffer hover_scroll_buffer({.width = 5,.height = 4});
    Painter hover_scroll_painter(hover_scroll_buffer);
    scrolling.root()->paint(hover_scroll_painter,{},std::to_string(scrolling.root()->id()));
    assert(hover_scroll_buffer.at(4,1).style.foreground != scroll_buffer.at(4,1).style.foreground);
    int grab = -1;
    assert(scrolling.root()->scrollbarPress({.x = 4,.y = 1},grab));
    assert(grab == 0);
    assert(scrolling.root()->scrollbarDrag(2,grab));
    assert(reported_scroll == 3);
    assert(scrolling.root()->scrollbarPress({.x = 4,.y = 0},grab));
    assert(reported_scroll == 0);
    assert(scrolling.root()->scrollTarget({.x = 1,.y = 1}) == scrolling.root());
    assert(scrolling.root()->scrollBy(3));
    assert(reported_scroll == 3);
    assert(scrolling.root()->scrollBy(-100));
    assert(reported_scroll == 0);
    assert(scrolling.root()->handleScrollKey(Event{.key = Key::PageDown}));
    assert(reported_scroll == 3);
    assert(scrolling.root()->handleScrollKey(Event{.key = Key::Up}));
    assert(reported_scroll == 1);
    assert(scrolling.root()->handleScrollKey(Event{.key = Key::Home}));
    assert(reported_scroll == 0);
    assert(scrolling.root()->handleScrollKey(Event{.key = Key::End}));
    assert(reported_scroll == 3);
    State<int> wheel_offset{0};
    Application wheel_app(std::make_unique<HeadlessTerminalBackend>(Size{.width = 8,.height = 3}));
    wheel_app.render([&] {
        return ScrollView({.children = {
            Typography({.text = "0"}),Typography({.text = "1"}),Typography({.text = "2"}),
            Typography({.text = "3"}),Typography({.text = "4"}),Typography({.text = "5"})
        },.layout = {.width = 8,.height = 3},.scrollOffset = wheel_offset.get(),
            .onScroll = [&](int next) { wheel_offset.set(next); }});
    });
    wheel_app.renderOnce();
    assert(wheel_app.focus().current() == std::to_string(wheel_app.reconciler().root()->id()));
    assert(wheel_app.dispatchEvent(Event{.key = Key::PageDown}));
    assert(wheel_offset.get() == 2);
    wheel_app.renderOnce();
    assert(wheel_app.dispatchEvent(Event{.key = Key::Home}));
    assert(wheel_offset.get() == 0);
    wheel_app.renderOnce();
    assert(wheel_app.dispatchEvent(Event{.key = Key::Mouse,.mouse = {
        .position = {.x = 1,.y = 1},.action = MouseAction::Scroll,.scrollY = -1}}));
    assert(wheel_offset.get() == 3);
    wheel_app.renderOnce();
    assert(wheel_app.reconciler().root()->children()[0]->bounds().y == -3);
    auto& wheel_pointer_output = dynamic_cast<HeadlessTerminalBackend&>(wheel_app.backend());
    wheel_pointer_output.clearOutput();
    wheel_app.dispatchEvent(Event{.key = Key::Mouse,.mouse = {
        .position = {.x = 7,.y = 2},.action = MouseAction::Move}});
    assert(wheel_pointer_output.output().find("\033]22;grab\033\\") != std::string::npos);
    wheel_pointer_output.clearOutput();
    assert(wheel_app.dispatchEvent(Event{.key = Key::Mouse,.mouse = {
        .position = {.x = 7,.y = 2},.button = MouseButton::Left,.action = MouseAction::Press}}));
    assert(wheel_pointer_output.output().find("\033]22;grabbing\033\\") != std::string::npos);
    assert(wheel_app.dispatchEvent(Event{.key = Key::Mouse,.mouse = {
        .position = {.x = 7,.y = 0},.button = MouseButton::Left,.action = MouseAction::Move}}));
    assert(wheel_offset.get() == 0);
    wheel_app.renderOnce();
    assert(wheel_app.dispatchEvent(Event{.key = Key::Mouse,.mouse = {
        .position = {.x = 30,.y = 100},.button = MouseButton::Left,.action = MouseAction::Move}}));
    assert(wheel_offset.get() == 3);
    assert(wheel_app.dispatchEvent(Event{.key = Key::Mouse,.mouse = {
        .position = {.x = 30,.y = 100},.button = MouseButton::Left,.action = MouseAction::Release}}));
    wheel_app.renderOnce();
    wheel_app.dispatchEvent(Event{.key = Key::Mouse,.mouse = {
        .position = {.x = 7,.y = 0},.action = MouseAction::Move}});
    assert(wheel_offset.get() == 3);

    Reconciler fields;
    fields.update(Column({.children = {
        TextField({.value = "Łódź",.key = "outlined",.layout = {.width = 16}}),
        TextField({.placeholder = "Search",.key = "filled",.layout = {.width = 16},
            .variant = TextFieldVariant::Filled}),
        TextField({.value = "Disabled",.key = "disabled",.disabled = true,
            .layout = {.width = 16}})
    }}));
    fields.root()->layout(Constraints{.maximum = {.width = 20,.height = 12}});
    ScreenBuffer fields_buffer({.width = 20,.height = 12});
    Painter fields_painter(fields_buffer);
    const Theme field_theme = Theme::light();
    const auto& field_nodes = fields.root()->children();
    fields.root()->paint(fields_painter,std::to_string(field_nodes[0]->id()),{},std::nullopt,&field_theme);
    assert(fields_buffer.at(0,0).grapheme == "┏");
    assert(fields_buffer.at(2,1).grapheme == "Ł");
    assert(fields_buffer.at(1,4).grapheme == "S");
    assert(fields_buffer.at(1,4).style.foreground == field_theme.palette.muted);
    assert(fields_buffer.at(0,4).style.background == field_theme.palette.surface);
    assert(fields_buffer.at(0,6).grapheme == "╭");
    field_nodes[0]->setCursorFromX(2);
    fields_buffer.clear();
    fields.root()->paint(fields_painter,std::to_string(field_nodes[0]->id()),{},std::nullopt,&field_theme);
    assert(fields_buffer.at(2,1).grapheme == "Ł");
    assert(fields_buffer.at(2,1).style.background == field_theme.palette.primary);
    assert(fields_buffer.at(2,1).style.foreground == field_theme.palette.on_primary);
    fields_buffer.clear();
    fields.root()->paint(fields_painter,{},{},std::nullopt,&field_theme);
    assert(fields_buffer.at(2,1).grapheme == "Ł");
    assert(fields_buffer.at(2,1).style.background != field_theme.palette.primary);
    const Color normal_field_background = fields_buffer.at(0,3).style.background;
    fields_buffer.clear();
    fields.root()->paint(fields_painter,{},std::to_string(field_nodes[1]->id()),std::nullopt,&field_theme);
    assert(fields_buffer.at(0,3).style.background != normal_field_background);

    std::string edited;
    fields.update(TextField({.value = "123456789",.key = "long",.layout = {.width = 8},
        .onChange = [&](std::string value) { edited = std::move(value); }}));
    fields.root()->layout(Constraints{.maximum = {.width = 8,.height = 3}});
    fields_buffer.clear();
    fields.root()->paint(fields_painter,std::to_string(fields.root()->id()),{},std::nullopt,&field_theme);
    assert(fields_buffer.at(2,1).grapheme == "7");
    fields.root()->setCursorFromX(2);
    assert(fields.root()->handleTextInput(Event{.key = Key::Backspace}));
    assert(edited == "12345789");

    Reconciler graphic_buttons;
    graphic_buttons.update(Row({.children = {
        Button({.text = "Save",.key = "save",.variant = ButtonVariant::Contained}),
        Button({.text = "Cancel",.key = "cancel",.variant = ButtonVariant::Outlined}),
        Button({.text = "Help",.key = "help",.variant = ButtonVariant::Text}),
        Button({.text = "Disabled",.key = "disabled",.variant = ButtonVariant::Contained,.disabled = true})
    },.layout = {.gap = 1}}));
    graphic_buttons.root()->layout(Constraints{.maximum = {.width = 80,.height = 5}});
    const auto& buttons = graphic_buttons.root()->children();
    assert(buttons[0]->bounds().height == 3);
    assert(buttons[1]->bounds().height == 3);
    ScreenBuffer buttons_buffer({.width = 80,.height = 5});
    Painter buttons_painter(buttons_buffer);
    const Theme dark = Theme::dark();
    graphic_buttons.root()->paint(buttons_painter,{},std::to_string(buttons[0]->id()),std::nullopt,&dark);
    assert(buttons_buffer.at(buttons[0]->bounds().x,1).style.background != dark.palette.primary);
    assert(buttons_buffer.at(buttons[1]->bounds().x,0).grapheme == "╭");
    assert(buttons_buffer.at(buttons[2]->bounds().x,0).grapheme != "╭");
    assert(buttons_buffer.at(buttons[3]->bounds().x,1).style.background == dark.palette.surface);
    buttons_buffer.clear();
    graphic_buttons.root()->paint(buttons_painter,std::to_string(buttons[1]->id()),{},std::nullopt,&dark);
    assert(buttons_buffer.at(buttons[1]->bounds().x,0).grapheme == "┏");

    Reconciler flexible;
    flexible.update(Column({.children = {
        Typography({.text = "Header"}),
        Row({.children = {
            Box({.children = {Typography({.text = "L"})},.layout = {.width = 5}}),
            Box({.children = {Typography({.text = "R"})},.layout = {.flex = 1}})
        },.layout = {.flex = 1,.alignItems = AlignItems::Stretch}}),
        Typography({.text = "Footer"})
    },.layout = {.width = 30,.height = 10,.alignItems = AlignItems::Stretch}}));
    flexible.root()->layout(Constraints{.maximum = {.width = 30,.height = 10}});
    const auto& flex_children = flexible.root()->children();
    assert(flex_children[0]->bounds().height == 1);
    assert(flex_children[1]->bounds().height == 8);
    assert(flex_children[2]->bounds().y == 9);
    assert(flex_children[1]->children()[0]->bounds().width == 5);
    assert(flex_children[1]->children()[1]->bounds().width == 25);
    assert(flex_children[1]->children()[0]->bounds().height == 8);

    flexible.update(Row({.children = {
        Typography({.text = "A"}),Typography({.text = "B"})
    },.layout = {.width = 12,.justifyContent = JustifyContent::SpaceBetween,
        .alignItems = AlignItems::End}}));
    flexible.root()->layout(Constraints{.maximum = {.width = 12,.height = 4}});
    assert(flexible.root()->children()[0]->bounds().x == 0);
    assert(flexible.root()->children()[1]->bounds().x == 11);

    flexible.update(Box({.children = {Typography({.text = "X"})},
        .layout = {.width = 20,.minWidth = 3,.maxWidth = 8}}));
    assert(flexible.root()->measure(Constraints{.maximum = {.width = 40,.height = 10}}).width == 8);

    const Theme paper_theme = Theme::light();
    Reconciler surfaces;
    surfaces.update(Stack({.children = {
        Paper({.children = {Typography({.text = "Paper"})},
            .layout = {.width = 14,.padding = 1}}),
        Card({.title = "System",.children = {Typography({.text = "Ready"})},
            .layout = {.width = 14,.padding = 1,.gap = 1}})
    }}));
    surfaces.root()->layout(Constraints{.maximum = {.width = 20,.height = 12}});
    ScreenBuffer surface_buffer({.width = 20,.height = 12});
    Painter surface_painter(surface_buffer);
    surfaces.root()->paint(surface_painter,{},{},std::nullopt,&paper_theme);
    assert(surface_buffer.at(0,0).style.background == paper_theme.palette.surface);
    assert(surface_buffer.at(1,1).grapheme == "P");
    const auto card_y = surfaces.root()->children()[1]->bounds().y;
    assert(surface_buffer.at(0,card_y).grapheme == "╭");
    assert(surface_buffer.at(2,card_y+2).grapheme == "S");
    assert(surface_buffer.at(2,card_y+4).grapheme == "R");
    assert(surface_buffer.at(2,card_y+2).style.background == paper_theme.palette.surface);
    surfaces.update(Paper({.children = {Typography({.text = "Custom"})},
        .variant = PaperVariant::Outlined,.background = Color::rgb(7,8,9)}));
    surfaces.root()->layout(Constraints{.maximum = {.width = 20,.height = 6}});
    surface_buffer.clear();
    surfaces.root()->paint(surface_painter,{},{},std::nullopt,&paper_theme);
    assert(surface_buffer.at(0,0).grapheme == "┌");
    assert(surface_buffer.at(2,2).style.background == Color::rgb(7,8,9));

    Reconciler grid;
    grid.update(Grid({.children = {
        Typography({.text = "A"}), Typography({.text = "B"}),
        Typography({.text = "C"}), Typography({.text = "D"}),
        Typography({.text = "E"})
    },.layout = {.width = 11,.gap = 1},.columns = 2}));
    grid.root()->layout(Constraints{.maximum = {.width = 11,.height = 10}});
    assert(grid.root()->children()[0]->bounds().x == 0);
    assert(grid.root()->children()[1]->bounds().x == 6);
    assert(grid.root()->children()[2]->bounds().y == 2);
    assert(grid.root()->children()[4]->bounds().y == 4);
    ScreenBuffer grid_buffer({.width = 11,.height = 6});
    Painter grid_painter(grid_buffer);
    grid.root()->paint(grid_painter);
    assert(grid_buffer.at(6,0).grapheme == "B");
    assert(grid_buffer.at(0,4).grapheme == "E");

    Reconciler stack;
    stack.update(Stack({.children = {Typography({.text = "A"}),Typography({.text = "B"})},
        .direction = StackDirection::Horizontal}));
    stack.root()->layout(Constraints{.maximum = {.width = 10,.height = 2}});
    assert(stack.root()->children()[1]->bounds().x == 1);

    const Theme light = Theme::light();
    assert(light.spacing(3) == 3);
    assert(light.palette.background == Color::rgb(248,249,251));
    Reconciler themed;
    themed.update(Component::column({
        Button({.text = "OK",.variant = ButtonVariant::Contained,.size = ButtonSize::Compact}),
        Progress({.value = 0.5,.layout = {.width = 4}})
    }));
    themed.root()->layout(Constraints{.maximum = {.width = 15,.height = 4}});
    ScreenBuffer theme_buffer({.width = 15,.height = 4});
    Painter theme_painter(theme_buffer);
    themed.root()->paint(theme_painter,{},{},std::nullopt,&light);
    assert(theme_buffer.at(0,0).style.background == light.palette.primary);
    assert(theme_buffer.at(0,0).style.foreground == light.palette.on_primary);
    assert(theme_buffer.at(0,1).style.foreground == light.palette.primary);
    assert(theme_buffer.at(3,1).style.foreground == light.palette.muted);

    render::Canvas canvas(4,8);
    canvas.point(0,0);
    canvas.point(1,3);
    ScreenBuffer canvas_buffer({.width = 2,.height = 2});
    Painter canvas_painter(canvas_buffer);
    canvas.paint(canvas_painter);
    assert(canvas_buffer.at(0,0).grapheme == "⢁");
    canvas.clear();
    canvas.rectangle({.x = 0,.y = 0,.width = 4,.height = 8});
    canvas.paint(canvas_painter);
    assert(canvas_buffer.at(0,0).grapheme != "⠀");
    canvas.clear();
    canvas.line({.x = 0,.y = 0},{.x = 3,.y = 7});
    canvas.circle({.x = 1,.y = 2},1);
    canvas.paint(canvas_painter);
    assert(canvas_buffer.at(0,0).grapheme != "⠀");

    ScreenBuffer drawing({.width = 12,.height = 6});
    Painter drawing_painter(drawing);
    drawing_painter.border({.x = 1,.y = 1,.width = 8,.height = 4},BorderStyle::Rounded);
    assert(drawing.at(1,1).grapheme == "╭");
    assert(drawing.at(8,1).grapheme == "╮");
    assert(drawing.at(1,4).grapheme == "╰");
    assert(drawing.at(8,4).grapheme == "╯");
    drawing_painter.line({.x = 2,.y = 2},{.x = 6,.y = 2});
    assert(drawing.at(4,2).grapheme == "─");
    drawing_painter.fillBackground({.x = 2,.y = 2,.width = 3,.height = 1},Color::rgb(3,5,7));
    assert(drawing.at(3,2).style.background == Color::rgb(3,5,7));
    Reconciler styled;
    styled.update(Box({.children = {
        Typography({.text = "Hello"}),
        Divider({.layout = {.width = 9}}),
        Progress({.value = 0.5,.layout = {.width = 8}})
    },.layout = {.padding = 1},.border = BorderStyle::Rounded,
        .background = Color::rgb(8,9,10)}));
    styled.root()->layout(Constraints{.maximum = {.width = 20,.height = 10}});
    ScreenBuffer styled_buffer({.width = 20,.height = 10});
    Painter styled_painter(styled_buffer);
    styled.root()->paint(styled_painter);
    assert(styled_buffer.at(0,0).grapheme == "╭");
    assert(styled_buffer.at(2,2).grapheme == "H");
    assert(styled_buffer.at(2,2).style.background == Color::rgb(8,9,10));
    assert(styled_buffer.at(2,3).grapheme == "─");
    assert(styled_buffer.at(2,4).grapheme == "█");
    assert(styled_buffer.at(6,4).grapheme == "░");

    Reconciler reconciler;
    reconciler.update(Component::column({
        Component::text("Alpha","a"),
        Component::text("Beta","b")
    }));
    const auto* root = reconciler.root();
    assert(root && root->children().size() == 2);
    const auto root_id = root->id();
    const auto a_id = root->children()[0]->id();
    const auto b_id = root->children()[1]->id();
    reconciler.update(Component::column({
        Component::text("Beta updated","b"),
        Component::text("Alpha","a")
    }));
    assert(reconciler.root()->id() == root_id);
    assert(reconciler.root()->children()[0]->id() == b_id);
    assert(reconciler.root()->children()[0]->text() == "Beta updated");
    assert(reconciler.root()->children()[1]->id() == a_id);
    reconciler.update(Component::column({Component::column({},"b")}));
    assert(reconciler.root()->children()[0]->id() != b_id);
    bool duplicate_rejected = false;
    try
    {
        reconciler.update(Component::column({Component::text("A","same"),Component::text("B","same")}));
    }
    catch (const std::invalid_argument&)
    {
        duplicate_rejected = true;
    }
    assert(duplicate_rejected);

    reconciler.update(Component::column({
        Component::row({Component::text("A"),Component::text("BC")},"row",LayoutStyle{.gap = 2}),
        Component::box({Component::text("X")},LayoutStyle{.width = 8,.padding = 1},"box")
    },"root",LayoutStyle{.gap = 1}));
    auto* layout_root = reconciler.root();
    assert(layout_root->measure(Constraints{.maximum = {.width = 40,.height = 20}}) == (Size{.width = 8,.height = 5}));
    layout_root->layout(Constraints{.maximum = {.width = 40,.height = 20}});
    assert(layout_root->children()[0]->children()[1]->bounds().x == 3);
    assert(layout_root->children()[1]->bounds().y == 2);
    assert(layout_root->children()[1]->children()[0]->bounds().x == 1);
    assert(layout_root->children()[1]->children()[0]->bounds().y == 3);
    ScreenBuffer layout_buffer({.width = 12,.height = 8});
    Painter layout_painter(layout_buffer);
    layout_root->paint(layout_painter);
    assert(layout_buffer.at(3,0).grapheme == "B");
    assert(layout_buffer.at(1,3).grapheme == "X");
    reconciler.update(Component::row({Component::text("界X"),Component::text("Z")}));
    layout_root = reconciler.root();
    layout_root->layout(Constraints{.maximum = {.width = 2,.height = 1}});
    assert(layout_root->bounds().width == 2);
    assert(layout_root->children()[0]->bounds().width == 2);
    assert(layout_root->children()[1]->bounds().width == 0);
    ScreenBuffer clipped({.width = 2,.height = 1});
    Painter clipped_painter(clipped);
    layout_root->paint(clipped_painter);
    assert(clipped.at(0,0).grapheme == "界");
    assert(clipped.at(1,0).width == 0);

    FocusManager focus;
    int first_calls = 0;
    int second_calls = 0;
    focus.add("first",[&](const Event&) { ++first_calls; return true; });
    focus.add("second",[&](const Event&) { ++second_calls; return true; });
    assert(focus.current() == "first");
    assert(focus.dispatch(Event{.key = Key::Character,.character = U'x'}));
    assert(first_calls == 1 && second_calls == 0);
    assert(focus.dispatch(Event{.key = Key::Tab}));
    assert(focus.current() == "second");
    assert(focus.dispatch(Event{.key = Key::Enter}));
    assert(second_calls == 1);
    assert(focus.previous() && focus.current() == "first");
    assert(focus.remove("first") && focus.current() == "second");
    assert(!focus.focus("missing"));

    InputDecoder decoder;
    decoder.feed("\033");
    assert(!decoder.nextEvent());
    assert(decoder.nextEvent(true)->key == Key::Escape);
    decoder.feed("\033[");
    assert(!decoder.nextEvent());
    decoder.feed("A");
    assert(decoder.nextEvent()->key == Key::Up);
    decoder.feed("\033[3~\033OD");
    assert(decoder.nextEvent()->key == Key::Delete);
    assert(decoder.nextEvent()->key == Key::Left);
    decoder.feed("\xC5");
    assert(!decoder.nextEvent());
    decoder.feed("\x81");
    const auto letter = decoder.nextEvent();
    assert(letter && letter->key == Key::Character && letter->character == U'Ł');
    decoder.feed("\t\r");
    assert(decoder.nextEvent()->key == Key::Tab);
    assert(decoder.nextEvent()->key == Key::Enter);
    decoder.feed("\xE2");
    assert(!decoder.nextEvent());
    assert(decoder.nextEvent(true)->character == 0xFFFD);
    decoder.feed("\033[1;5C");
    assert(decoder.nextEvent()->key == Key::Right);
    decoder.feed("\033[99~");
    assert(decoder.nextEvent()->key == Key::Unknown);
    assert(!decoder.nextEvent());
    decoder.feed("\033[<0;2;");
    assert(!decoder.nextEvent());
    decoder.feed("3M");
    const auto mouse_press = decoder.nextEvent();
    assert(mouse_press && mouse_press->key == Key::Mouse);
    assert(mouse_press->mouse.action == MouseAction::Press);
    assert(mouse_press->mouse.button == MouseButton::Left);
    assert(mouse_press->mouse.position == (Point{.x = 1,.y = 2}));
    decoder.feed("\033[<0;2;3m\033[<64;5;6M");
    assert(decoder.nextEvent()->mouse.action == MouseAction::Release);
    const auto wheel = decoder.nextEvent();
    assert(wheel && wheel->mouse.action == MouseAction::Scroll && wheel->mouse.scrollY == 1);
    decoder.feed("\033[<32;3;4M");
    assert(decoder.nextEvent()->mouse.action == MouseAction::Move);

    ScreenBuffer screen({.width = 12,.height = 3});
    Painter painter(screen);
    painter.text(0,0,"Łódź");
    assert(screen.at(0,0).grapheme == "Ł");
    assert(screen.at(3,0).grapheme == "ź");
    painter.text(0,1,"界");
    assert(screen.at(0,1).width == 2);
    assert(screen.at(1,1).width == 0);
    painter.put(1,1,"A");
    assert(screen.at(0,1).grapheme == " ");
    assert(screen.at(1,1).grapheme == "A");
    painter.text(3,1,"e\xCC\x81");
    assert(screen.at(3,1).grapheme == "e\xCC\x81");
    painter.text(11,1,"界");
    assert(screen.at(11,1).grapheme == " ");
    painter.text(11,1,"A\xCC\x81");
    assert(screen.at(11,1).grapheme == "A\xCC\x81");
    painter.styleAt(1,0,TextStyle{.foreground = Color::rgb(1,2,3),.background = Color::rgb(4,5,6)});
    assert(screen.at(1,0).grapheme == "ó");
    assert(screen.at(1,0).style.background == Color::rgb(4,5,6));
    painter.text(4,2,"界");
    painter.styleAt(5,2,TextStyle{.foreground = Color::rgb(3,2,1),.background = Color::rgb(6,5,4)});
    assert(screen.at(4,2).grapheme == "界");
    assert(screen.at(4,2).style.background == Color::rgb(6,5,4));
    assert(screen.at(5,2).style.background == Color::rgb(6,5,4));
    assert(isValidUtf8("Łódź"));
    assert(!isValidUtf8("\xFF"));
    HeadlessTerminalBackend terminal({12,3});
    terminal.resize({.width = 14,.height = 4});
    const auto resized = terminal.pollEvent();
    assert(resized && resized->key == Key::Resize);
    assert(resized->size == (Size{.width = 14,.height = 4}));
    terminal.resize({.width = 12,.height = 3});
    assert(terminal.pollEvent()->key == Key::Resize);
    DifferentialRenderer renderer;
    renderer.render(screen,terminal);
    assert(!terminal.output().empty());
    terminal.clearOutput();
    renderer.render(screen,terminal);
    assert(terminal.output().empty());
    painter.put(2,1,"B");
    renderer.render(screen,terminal);
    assert(terminal.output().find("B") != std::string::npos);
    terminal.clearOutput();
    painter.text(5,1,"XY");
    renderer.render(screen,terminal);
    assert(terminal.output().find("XY") != std::string::npos);
    assert(terminal.output().find("\033[2J") == std::string::npos);
    assert(terminal.output().find("\033[2;7H") == std::string::npos);
    terminal.clearOutput();
    painter.text(7,1,"界");
    renderer.render(screen,terminal);
    terminal.clearOutput();
    painter.put(8,1,"Z");
    renderer.render(screen,terminal);
    assert(terminal.output().find(" Z") != std::string::npos);
    assert(screen.at(7,1).grapheme == " ");
    assert(screen.at(8,1).grapheme == "Z");
    auto backend = std::make_unique<HeadlessTerminalBackend>(Size{20,5});
    auto* observed = backend.get();
    Application app(std::move(backend));
    app.render([](Painter& p, Size) { p.text(0,0,"Terion"); });
    app.renderOnce();
    assert(observed->output().find("Terion") != std::string::npos);
    observed->clearOutput();
    std::string label = "First";
    app.render([&] {
        return Component::column({Component::text(label,"label")});
    });
    app.renderOnce();
    assert(observed->output().find("F") != std::string::npos);
    const auto label_id = app.reconciler().root()->children()[0]->id();
    ScreenBuffer component_buffer({.width = 20,.height = 5});
    Painter component_painter(component_buffer);
    app.reconciler().root()->paint(component_painter);
    assert(component_buffer.at(0,0).grapheme == "F");
    label = "Next";
    observed->clearOutput();
    app.renderOnce();
    assert(app.reconciler().root()->children()[0]->id() == label_id);
    assert(observed->output().find("N") != std::string::npos);
    State<int> counter{0};
    observed->clearOutput();
    app.render([&] { return Component::text(std::to_string(counter.get()),"counter"); });
    assert(app.needsRender());
    app.renderOnce();
    assert(!app.needsRender());
    const auto counter_id = app.reconciler().root()->id();
    counter.set(1);
    assert(app.needsRender());
    observed->clearOutput();
    app.renderOnce();
    assert(observed->output().find("1") != std::string::npos);
    assert(app.reconciler().root()->id() == counter_id);
    assert(!app.needsRender());
    counter.set(1);
    assert(!app.needsRender());
    counter.update([](const int value) { return value + 1; });
    assert(counter.get() == 2 && app.needsRender());
    State<bool> show{true};
    State<int> optional_value{10};
    app.render([&] {
        if (show.get()) return Component::text(std::to_string(optional_value.get()));
        return Component::text("hidden");
    });
    app.renderOnce();
    assert(!app.needsRender());
    show.set(false);
    assert(app.needsRender());
    app.renderOnce();
    assert(!app.needsRender());
    optional_value.set(11);
    assert(!app.needsRender());
    show.set(true);
    assert(app.needsRender());
    int clicks = 0;
    app.render([&] {
        return Column({.children = {
            Typography({.text = "Actions",.variant = TypographyVariant::H2}),
            Button({.text = "First",.key = "first",.onClick = [&] { ++clicks; },.size = ButtonSize::Compact}),
            Button({.text = "Disabled",.key = "disabled",.disabled = true,.onClick = [&] { clicks += 100; },
                .size = ButtonSize::Compact}),
            Button({.text = "Second",.key = "second",.onClick = [&] { clicks += 10; },.size = ButtonSize::Compact})
        }});
    });
    app.renderOnce();
    const auto first_button_id = std::to_string(app.reconciler().root()->children()[1]->id());
    const auto second_button_id = std::to_string(app.reconciler().root()->children()[3]->id());
    assert(app.focus().current() == first_button_id);
    assert(app.dispatchEvent(Event{.key = Key::Enter}));
    assert(clicks == 1);
    assert(app.dispatchEvent(Event{.key = Key::Tab}));
    assert(app.focus().current() == second_button_id);
    assert(app.dispatchEvent(Event{.key = Key::Character,.character = U' '}));
    assert(clicks == 11);
    app.renderOnce();
    assert(app.focus().current() == second_button_id);
    assert(app.dispatchEvent(Event{.key = Key::Mouse,.mouse = {
        .position = {.x = 2,.y = 1},.button = MouseButton::Left,.action = MouseAction::Press}}));
    assert(app.focus().current() == first_button_id);
    assert(app.dispatchEvent(Event{.key = Key::Mouse,.mouse = {
        .position = {.x = 2,.y = 1},.button = MouseButton::Left,.action = MouseAction::Release}}));
    assert(clicks == 12);
    app.dispatchEvent(Event{.key = Key::Mouse,.mouse = {
        .position = {.x = 2,.y = 2},.button = MouseButton::Left,.action = MouseAction::Press}});
    app.dispatchEvent(Event{.key = Key::Mouse,.mouse = {
        .position = {.x = 2,.y = 2},.button = MouseButton::Left,.action = MouseAction::Release}});
    assert(clicks == 12);
    app.dispatchEvent(Event{.key = Key::Mouse,.mouse = {
        .position = {.x = 2,.y = 1},.button = MouseButton::Left,.action = MouseAction::Press}});
    app.dispatchEvent(Event{.key = Key::Mouse,.mouse = {
        .position = {.x = 40,.y = 10},.button = MouseButton::Left,.action = MouseAction::Release}});
    assert(clicks == 12);
    ScreenBuffer hover_buffer({.width = 20,.height = 5});
    Painter hover_painter(hover_buffer);
    app.reconciler().root()->paint(hover_painter,{},first_button_id);
    assert(hover_buffer.at(0,1).grapheme == "▏");
    State<std::string> input{std::string{}};
    app.render([&] {
        return Column({.children = {
            TextField({.value = input.get(),.placeholder = "Name",.key = "name",
                .onChange = [&](std::string value) { input.set(std::move(value)); },.size = ButtonSize::Compact}),
            Button({.text = "OK",.key = "ok",.onClick = [&] { ++clicks; },.size = ButtonSize::Compact})
        }});
    });
    app.renderOnce();
    const auto field_id = std::to_string(app.reconciler().root()->children()[0]->id());
    assert(app.focus().current() == field_id);
    assert(app.dispatchEvent(Event{.key = Key::Character,.character = U'Ł'}));
    assert(input.get() == "Ł");
    app.renderOnce();
    assert(app.dispatchEvent(Event{.key = Key::Character,.character = U'x'}));
    assert(input.get() == "Łx");
    app.renderOnce();
    app.dispatchEvent(Event{.key = Key::Left});
    app.dispatchEvent(Event{.key = Key::Backspace});
    assert(input.get() == "x");
    app.renderOnce();
    assert(app.reconciler().root()->children()[0]->id() == std::stoull(field_id));
    app.dispatchEvent(Event{.key = Key::Mouse,.mouse = {
        .position = {.x = 1,.y = 0},.button = MouseButton::Left,.action = MouseAction::Press}});
    app.dispatchEvent(Event{.key = Key::Character,.character = U'Z'});
    assert(input.get() == "Zx");
    app.renderOnce();
    input.set("e\xCC\x81");
    app.renderOnce();
    app.dispatchEvent(Event{.key = Key::Mouse,.mouse = {
        .position = {.x = 3,.y = 0},.button = MouseButton::Left,.action = MouseAction::Press}});
    app.dispatchEvent(Event{.key = Key::Backspace});
    assert(input.get().empty());
    app.renderOnce();
    assert(app.dispatchEvent(Event{.key = Key::Tab}));
    assert(app.dispatchEvent(Event{.key = Key::Enter}));
    assert(clicks == 13);
    app.render([](Painter&, Size) {});
    app.renderOnce();
    bool received = false;
    app.onEvent([&](const Event& event) { received = event.key == Key::Enter; return received; });
    assert(app.dispatchEvent(Event{.key = Key::Enter}));
    assert(received);
    received = false;
    app.focus().add("editor",[&](const Event& event) { return event.key == Key::Escape; });
    assert(app.dispatchEvent(Event{.key = Key::Escape}));
    assert(!received);
    assert(app.focus().remove("editor"));
    assert(app.dispatchEvent(Event{.key = Key::Escape}));
    // The headless backend allows run() to finish deterministically without a TTY.
    observed->push(Event{.key = Key::Escape});
    assert(app.run() == 0);
    std::cout << "Terion tests passed\n";
}
