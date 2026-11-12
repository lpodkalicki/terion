/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#ifndef TERION_COMPONENT_H
#define TERION_COMPONENT_H

#include <string>
#include <functional>
#include <optional>
#include <utility>
#include <vector>
#include <terion/layout.h>
#include <terion/event.h>
#include <terion/render/color.h>
#include <terion/theme.h>
#include <terion/form_value.h>
#include <terion/styled_text.h>

namespace terion {

enum class ButtonVariant
{
    Text,
    Outlined,
    Contained
}; /* End of enum ButtonVariant */

enum class ButtonSize
{
    Compact,
    Medium,
    Large
}; /* End of enum ButtonSize */

enum class CheckboxVariant
{
    Default,
    Unicode,
    Emoji
}; /* End of enum CheckboxVariant */

enum class TextFieldVariant
{
    Outlined,
    Filled
}; /* End of enum TextFieldVariant */

enum class SurfaceTone
{
    None,
    Surface,
    Primary
}; /* End of enum SurfaceTone */

struct SelectOption
{
    std::string label;
    std::string value;
    bool disabled = false;
}; /* End of struct SelectOption */

struct RadioOption
{
    std::string label;
    std::string value;
    bool disabled = false;
}; /* End of struct RadioOption */

struct TabOption
{
    std::string label;
    std::string value;
    bool disabled = false;
}; /* End of struct TabOption */

enum class TableAlignment
{
    Left,
    Center,
    Right
}; /* End of enum TableAlignment */

enum class SplitDirection
{
    Horizontal,
    Vertical
}; /* End of enum SplitDirection */

enum class ProgressVariant
{
    Blocks,
    Line,
    Ascii
}; /* End of enum ProgressVariant */

struct TableColumn
{
    std::string title;
    int width = 0;
    TableAlignment alignment = TableAlignment::Left;
}; /* End of struct TableColumn */

struct TableRow
{
    std::string key;
    std::vector<std::string> cells;
    bool disabled = false;
}; /* End of struct TableRow */

struct TableCellFormat
{
    std::string text;
    std::optional<Color> foreground = std::nullopt;
    std::optional<Color> background = std::nullopt;
    bool bold = false;
}; /* End of struct TableCellFormat */

struct TreeItem
{
    std::string key;
    std::string label;
    std::vector<TreeItem> children = {};
    bool disabled = false;
    std::string icon = {};
}; /* End of struct TreeItem */

class Component
{
public:
    enum class Type
    {
        Text,
        Column,
        Row,
        Box,
        Grid,
        ScrollView,
        Button,
        Checkbox,
        Switch,
        Radio,
        RadioGroup,
        TextField,
        TextArea,
        NumberField,
        PasswordField,
        Select,
        Autocomplete,
        DropdownMenu,
        ContextMenu,
        Tooltip,
        Toast,
        Tabs,
        TabPanel,
        Table,
        TreeView,
        VirtualList,
        ResizableSplit,
        Dialog,
        FocusScope,
        Divider,
        Progress,
        Slider,
        Function,
        ContextProvider,
        Fragment,
        ErrorBoundary,
        ThemeScope,
        FormFieldMarker
    }; /* End of enum Type */

    struct Props
    {
        Type type = Type::Column;
        std::string key = {};
        std::string text = {};
        std::vector<Component> children = {};
        std::function<Component()> render_function = {};
        const void* function_identity = nullptr;
        std::function<void()> enter_context = {};
        std::function<void()> exit_context = {};
        std::function<Component(const std::string&)> error_fallback = {};
        std::optional<Theme> theme_override = std::nullopt;
        LayoutStyle layout = {};
        TextStyle text_style = {};
        ColorVariant color_variant = ColorVariant::Default;
        std::function<void()> on_click = {};
        std::function<void(bool)> on_toggle = {};
        std::function<void(ComponentEvent&)> on_event_capture = {};
        std::function<void(ComponentEvent&)> on_event = {};
        std::function<void(std::string)> on_change = {};
        std::string placeholder = {};
        bool disabled = false;
        bool read_only = false;
        int text_area_min_rows = 4;
        bool password_reveal = false;
        char32_t password_mask = U'•';
        double number_min = -1.0e100;
        double number_max = 1.0e100;
        double number_step = 1.0;
        std::optional<BorderStyle> border = std::nullopt;
        std::optional<Color> background = std::nullopt;
        ButtonVariant button_variant = ButtonVariant::Outlined;
        double value = 0.0;
        int columns = 1;
        SurfaceTone surface_tone = SurfaceTone::None;
        ButtonSize button_size = ButtonSize::Medium;
        CheckboxVariant checkbox_variant = CheckboxVariant::Default;
        TextFieldVariant text_field_variant = TextFieldVariant::Outlined;
        int scroll_offset = 0;
        std::function<void(int)> on_scroll = {};
        bool scrollbar_visible = true;
        std::vector<SelectOption> select_options = {};
        std::string autocomplete_query = {};
        std::function<void(std::string)> autocomplete_on_input_change = {};
        std::vector<RadioOption> radio_options = {};
        std::vector<TabOption> tab_options = {};
        std::vector<TableColumn> table_columns = {};
        std::vector<TableRow> table_rows = {};
        std::function<bool(const TableRow&)> table_filter = {};
        std::function<TableCellFormat(const TableRow&,std::size_t)> table_format_cell = {};
        int virtual_item_count = 0;
        std::function<std::string(int)> virtual_item_text = {};
        std::function<StyledText(int)> virtual_item_styled = {};
        int virtual_selected_index = -1;
        bool virtual_follow_tail = false;
        std::function<void(int)> virtual_on_select = {};
        std::vector<TreeItem> tree_items = {};
        std::vector<std::string> tree_expanded = {};
        std::function<void(std::string,bool)> tree_on_toggle = {};
        SplitDirection split_direction = SplitDirection::Horizontal;
        int split_position = 20;
        int split_min_first = 3;
        int split_min_second = 3;
        std::function<void(int)> split_on_change = {};
        bool tab_panel_active = true;
        bool checked = false;
        bool dialog_open = false;
        std::string dialog_title = {};
        std::string dialog_confirm_text = "Confirm";
        std::string dialog_cancel_text = "Cancel";
        int dialog_width = 48;
        bool dialog_close_on_backdrop = true;
        std::function<void()> dialog_on_close = {};
        bool dialog_show_actions = true;
        bool focus_scope_active = false;
        bool toast_open = false;
        int toast_duration_ms = 3500;
        int toast_severity = 0;
        std::function<void()> toast_on_close = {};
        ProgressVariant progress_variant = ProgressVariant::Blocks;
        bool progress_indeterminate = false;
        bool progress_show_percentage = false;
        double slider_min = 0.0;
        double slider_max = 100.0;
        double slider_step = 1.0;
        bool slider_show_value = false;
        std::function<void(double)> slider_on_change = {};
        std::optional<FormValue> form_default_value = std::nullopt;
        std::function<std::optional<std::string>(const FormValue&)> form_validator = {};
        std::string form_field_name = {};
        std::string form_field_label = {};
        bool form_field_required = false;
    }; /* End of struct Props */

    explicit Component(Props props) : props_(std::move(props)) {}

    static Component text(std::string value, std::string key = {});
    static Component column(std::vector<Component> children, std::string key = {}, LayoutStyle layout = {});
    static Component row(std::vector<Component> children, std::string key = {}, LayoutStyle layout = {});
    static Component box(std::vector<Component> children, LayoutStyle layout = {}, std::string key = {});

    [[nodiscard]] const Props& props() const noexcept { return props_; }

    [[nodiscard]] Component keyed(std::string key) const &
    {
        auto props = props_;
        props.key = std::move(key);
        return Component(std::move(props));
    }

    [[nodiscard]] Component keyed(std::string key) &&
    {
        props_.key = std::move(key);
        return std::move(*this);
    }

private:
    Props props_;
}; /* End of class Component */

} /* End of namespace terion */

#endif /* TERION_COMPONENT_H */
