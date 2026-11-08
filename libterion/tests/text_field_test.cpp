/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <terion/terion.h>
#include <gtest/gtest.h>

using namespace terion;

TEST(TextField, RendersCursorAndEditsUtf8Text)
{
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
    EXPECT_EQ(fields_buffer.at(0,0).grapheme,"┏");
    EXPECT_EQ(fields_buffer.at(2,1).grapheme,"Ł");
    EXPECT_EQ(fields_buffer.at(1,4).grapheme,"S");
    EXPECT_EQ(fields_buffer.at(1,4).style.foreground,field_theme.palette.muted);
    EXPECT_EQ(fields_buffer.at(0,4).style.background,field_theme.palette.surface);
    EXPECT_EQ(fields_buffer.at(0,6).grapheme,"╭");
    field_nodes[0]->setCursorFromX(2);
    fields_buffer.clear();
    fields.root()->paint(fields_painter,std::to_string(field_nodes[0]->id()),{},std::nullopt,&field_theme);
    EXPECT_EQ(fields_buffer.at(2,1).grapheme,"Ł");
    EXPECT_EQ(fields_buffer.at(2,1).style.background,field_theme.palette.primary);
    EXPECT_EQ(fields_buffer.at(2,1).style.foreground,field_theme.palette.on_primary);
    fields_buffer.clear();
    fields.root()->paint(fields_painter,{},{},std::nullopt,&field_theme);
    EXPECT_EQ(fields_buffer.at(2,1).grapheme,"Ł");
    EXPECT_NE(fields_buffer.at(2,1).style.background,field_theme.palette.primary);
    const Color normal_field_background = fields_buffer.at(0,3).style.background;
    fields_buffer.clear();
    fields.root()->paint(fields_painter,{},std::to_string(field_nodes[1]->id()),std::nullopt,&field_theme);
    EXPECT_NE(fields_buffer.at(0,3).style.background,normal_field_background);

    std::string edited;
    fields.update(TextField({.value = "123456789",.key = "long",.layout = {.width = 8},
        .onChange = [&](std::string value) { edited = std::move(value); }}));
    fields.root()->layout(Constraints{.maximum = {.width = 8,.height = 3}});
    fields_buffer.clear();
    fields.root()->paint(fields_painter,std::to_string(fields.root()->id()),{},std::nullopt,&field_theme);
    EXPECT_EQ(fields_buffer.at(2,1).grapheme,"7");
    fields.root()->setCursorFromX(2);
    EXPECT_TRUE(fields.root()->handleTextInput(Event{.key = Key::Backspace}));
    EXPECT_EQ(edited,"12345789");

}
