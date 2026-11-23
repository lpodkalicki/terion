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
#include <terion/detail/data_slot.h>

namespace terion {

enum class ButtonVariant
{
    Text,
    Outlined,
    Contained
}; /* End of enum ButtonVariant */

enum class CheckboxVariant
{
    Default,
    Unicode,
    Emoji
}; /* End of enum CheckboxVariant */

enum class InputVariant
{
    Outlined,
    Filled
}; /* End of enum InputVariant */


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
    std::optional<TextStyleSpec> style_spec = std::nullopt;
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

    struct TextData
    {
        std::string text = {};
        TextStyle text_style = {};
        ColorVariant color_variant = ColorVariant::Default;
        std::string placeholder = {};
        std::optional<TextStyleSpec> text_style_spec = std::nullopt;
    }; /* End of struct TextData */

    struct InputData
    {
        bool read_only = false;
        int text_area_min_rows = 4;
        bool text_area_soft_wrap = false;
        bool password_reveal = false;
        char32_t password_mask = U'•';
        double number_min = -1.0e100;
        double number_max = 1.0e100;
        double number_step = 1.0;
        ControlSize control_size = ControlSize::Medium;
        InputVariant input_variant = InputVariant::Outlined;
        std::optional<FormValue> form_default_value = std::nullopt;
        std::function<std::optional<std::string>(const FormValue&)> form_validator = {};
        bool uncontrolled_value = false;
    }; /* End of struct InputData */

    struct ActionData
    {
        std::function<void()> on_click = {};
        std::function<void(bool)> on_toggle = {};
        std::function<void(std::string)> on_change = {};
        ButtonVariant button_variant = ButtonVariant::Outlined;
        double value = 0.0;
        CheckboxVariant checkbox_variant = CheckboxVariant::Default;
        bool checked = false;
    }; /* End of struct ActionData */

    struct SurfaceData
    {
        std::optional<BorderStyle> border = std::nullopt;
        std::optional<Color> background = std::nullopt;
        int columns = 1;
        SurfaceTone surface_tone = SurfaceTone::None;
    }; /* End of struct SurfaceData */

    struct ScrollData
    {
        int scroll_offset = 0;
        std::function<void(int)> on_scroll = {};
        bool scrollbar_visible = true;
    }; /* End of struct ScrollData */

    struct ChoiceData
    {
        std::vector<SelectOption> select_options = {};
        std::string autocomplete_query = {};
        std::function<void(std::string)> autocomplete_on_input_change = {};
        std::vector<RadioOption> radio_options = {};
        std::vector<TabOption> tab_options = {};
        bool tab_panel_active = true;
    }; /* End of struct ChoiceData */

    struct TableData
    {
        std::vector<TableColumn> table_columns = {};
        std::vector<TableRow> table_rows = {};
        std::function<bool(const TableRow&)> table_filter = {};
        std::function<TableCellFormat(const TableRow&,std::size_t)> table_format_cell = {};
        int table_horizontal_offset = -1;
        std::function<void(int)> table_on_horizontal_scroll = {};
    }; /* End of struct TableData */

    struct VirtualListData
    {
        int virtual_item_count = 0;
        std::function<std::string(int)> virtual_item_text = {};
        std::function<StyledText(int)> virtual_item_styled = {};
        std::function<std::pair<int,int>(int,int)> virtual_on_width_change = {};
        int virtual_selected_index = -1;
        std::function<bool(int)> virtual_item_is_selected = {};
        int virtual_row_height = 1;
        bool virtual_follow_tail = false;
        std::function<void(int)> virtual_on_select = {};
        std::function<void(int)> virtual_on_activate = {};
        std::function<void(int,int)> virtual_on_click_position = {};
    }; /* End of struct VirtualListData */

    struct TreeData
    {
        std::vector<TreeItem> tree_items = {};
        std::vector<std::string> tree_expanded = {};
        std::function<void(std::string,bool)> tree_on_toggle = {};
    }; /* End of struct TreeData */

    struct SplitData
    {
        SplitDirection split_direction = SplitDirection::Horizontal;
        int split_position = 20;
        int split_min_first = 3;
        int split_min_second = 3;
        std::function<void(int)> split_on_change = {};
    }; /* End of struct SplitData */

    struct OverlayData
    {
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
    }; /* End of struct OverlayData */

    struct ProgressData
    {
        ProgressVariant progress_variant = ProgressVariant::Blocks;
        bool progress_indeterminate = false;
        bool progress_show_percentage = false;
    }; /* End of struct ProgressData */

    struct SliderData
    {
        double slider_min = 0.0;
        double slider_max = 100.0;
        double slider_step = 1.0;
        bool slider_show_value = false;
        std::function<void(double)> slider_on_change = {};
    }; /* End of struct SliderData */

    struct FormData
    {
        std::string form_field_name = {};
        std::string form_field_label = {};
        bool form_field_required = false;
        std::function<std::optional<std::string>(const FormValue&,const FormValues&)> form_field_validator = {};
    }; /* End of struct FormData */

    struct RenderFunctionData
    {
        std::function<Component()> render_function = {};
        const void* function_identity = nullptr;
        std::function<void()> enter_context = {};
        std::function<void()> exit_context = {};
        std::function<Component(const std::string&)> error_fallback = {};
    }; /* End of struct RenderFunctionData */

    struct Props
    {
        Type type = Type::Column;
        std::string key = {};
        std::string id = {};
        std::vector<Component> children = {};
        LayoutStyle layout = {};
        std::optional<Theme> theme_override = std::nullopt;
        std::function<void(ComponentEvent&)> on_event_capture = {};
        std::function<void(ComponentEvent&)> on_event = {};
        bool disabled = false;
        std::function<void()> on_blur = {};

        detail::DataSlot<TextData> text = {};
        detail::DataSlot<InputData> input = {};
        detail::DataSlot<ActionData> action = {};
        detail::DataSlot<SurfaceData> surface = {};
        detail::DataSlot<ScrollData> scroll = {};
        detail::DataSlot<ChoiceData> choice = {};
        detail::DataSlot<TableData> table = {};
        detail::DataSlot<VirtualListData> virtual_list = {};
        detail::DataSlot<TreeData> tree = {};
        detail::DataSlot<SplitData> split = {};
        detail::DataSlot<OverlayData> overlay = {};
        detail::DataSlot<ProgressData> progress = {};
        detail::DataSlot<SliderData> slider = {};
        detail::DataSlot<FormData> form = {};
        detail::DataSlot<RenderFunctionData> render_function = {};

        [[nodiscard]] const TextData& textData() const noexcept
        {
            static const TextData defaults{};
            return text ? *text : defaults;
        }
        TextData& textData()
        {
            if (!text) text.emplace();
            return *text;
        }
        [[nodiscard]] const InputData& inputData() const noexcept
        {
            static const InputData defaults{};
            return input ? *input : defaults;
        }
        InputData& inputData()
        {
            if (!input) input.emplace();
            return *input;
        }
        [[nodiscard]] const ActionData& actionData() const noexcept
        {
            static const ActionData defaults{};
            return action ? *action : defaults;
        }
        ActionData& actionData()
        {
            if (!action) action.emplace();
            return *action;
        }
        [[nodiscard]] const SurfaceData& surfaceData() const noexcept
        {
            static const SurfaceData defaults{};
            return surface ? *surface : defaults;
        }
        SurfaceData& surfaceData()
        {
            if (!surface) surface.emplace();
            return *surface;
        }
        [[nodiscard]] const ScrollData& scrollData() const noexcept
        {
            static const ScrollData defaults{};
            return scroll ? *scroll : defaults;
        }
        ScrollData& scrollData()
        {
            if (!scroll) scroll.emplace();
            return *scroll;
        }
        [[nodiscard]] const ChoiceData& choiceData() const noexcept
        {
            static const ChoiceData defaults{};
            return choice ? *choice : defaults;
        }
        ChoiceData& choiceData()
        {
            if (!choice) choice.emplace();
            return *choice;
        }
        [[nodiscard]] const TableData& tableData() const noexcept
        {
            static const TableData defaults{};
            return table ? *table : defaults;
        }
        TableData& tableData()
        {
            if (!table) table.emplace();
            return *table;
        }
        [[nodiscard]] const VirtualListData& virtualListData() const noexcept
        {
            static const VirtualListData defaults{};
            return virtual_list ? *virtual_list : defaults;
        }
        VirtualListData& virtualListData()
        {
            if (!virtual_list) virtual_list.emplace();
            return *virtual_list;
        }
        [[nodiscard]] const TreeData& treeData() const noexcept
        {
            static const TreeData defaults{};
            return tree ? *tree : defaults;
        }
        TreeData& treeData()
        {
            if (!tree) tree.emplace();
            return *tree;
        }
        [[nodiscard]] const SplitData& splitData() const noexcept
        {
            static const SplitData defaults{};
            return split ? *split : defaults;
        }
        SplitData& splitData()
        {
            if (!split) split.emplace();
            return *split;
        }
        [[nodiscard]] const OverlayData& overlayData() const noexcept
        {
            static const OverlayData defaults{};
            return overlay ? *overlay : defaults;
        }
        OverlayData& overlayData()
        {
            if (!overlay) overlay.emplace();
            return *overlay;
        }
        [[nodiscard]] const ProgressData& progressData() const noexcept
        {
            static const ProgressData defaults{};
            return progress ? *progress : defaults;
        }
        ProgressData& progressData()
        {
            if (!progress) progress.emplace();
            return *progress;
        }
        [[nodiscard]] const SliderData& sliderData() const noexcept
        {
            static const SliderData defaults{};
            return slider ? *slider : defaults;
        }
        SliderData& sliderData()
        {
            if (!slider) slider.emplace();
            return *slider;
        }
        [[nodiscard]] const FormData& formData() const noexcept
        {
            static const FormData defaults{};
            return form ? *form : defaults;
        }
        FormData& formData()
        {
            if (!form) form.emplace();
            return *form;
        }
        [[nodiscard]] const RenderFunctionData& renderFunctionData() const noexcept
        {
            static const RenderFunctionData defaults{};
            return render_function ? *render_function : defaults;
        }
        RenderFunctionData& renderFunctionData()
        {
            if (!render_function) render_function.emplace();
            return *render_function;
        }
    }; /* End of struct Props */

    explicit Component(Props props) : props_(std::move(props)) {}

    static Component text(std::string value, std::string key = {});
    static Component column(std::vector<Component> children, std::string key = {}, LayoutStyle layout = {});
    static Component row(std::vector<Component> children, std::string key = {}, LayoutStyle layout = {});
    static Component box(std::vector<Component> children, LayoutStyle layout = {}, std::string key = {});

    [[nodiscard]] const Props& props() const noexcept { return props_; }

    [[nodiscard]] Component identified(std::string id) const &
    {
        auto props = props_;
        props.id = std::move(id);
        return Component(std::move(props));
    }

    [[nodiscard]] Component identified(std::string id) &&
    {
        props_.id = std::move(id);
        return std::move(*this);
    }

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
