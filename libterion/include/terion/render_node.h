/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#ifndef TERION_RENDER_NODE_H
#define TERION_RENDER_NODE_H

#include <cstdint>
#include <chrono>
#include <memory>
#include <string>
#include <functional>
#include <string_view>
#include <vector>
#include <terion/component.h>
#include <terion/layout.h>
#include <terion/scroll_controller.h>
#include <terion/render/painter.h>
#include <terion/terminal.h>
#include <terion/theme.h>

namespace terion {

class RenderNode
{
public:
    [[nodiscard]] std::uint64_t id() const noexcept { return id_; }
    [[nodiscard]] Component::Type type() const noexcept { return type_; }
    [[nodiscard]] const std::string& key() const noexcept { return key_; }
    [[nodiscard]] const std::string& text() const noexcept { return text_; }
    [[nodiscard]] const Theme& resolvedTheme(const Theme& fallback) const noexcept
    {
        return theme_override_ ? *theme_override_ : fallback;
    }
    [[nodiscard]] const std::vector<std::unique_ptr<RenderNode>>& children() const noexcept { return children_; }
    [[nodiscard]] Rect bounds() const noexcept { return bounds_; }
    [[nodiscard]] bool propagateEvent(const Event& event, std::uint64_t target) const;
    [[nodiscard]] RenderNode* findById(std::uint64_t id) noexcept;
    [[nodiscard]] const RenderNode* findById(std::uint64_t id) const noexcept;
    [[nodiscard]] const RenderNode* hitTest(Point position) const noexcept;
    [[nodiscard]] RenderNode* hitTest(Point position) noexcept
    {
        return const_cast<RenderNode*>(static_cast<const RenderNode&>(*this).hitTest(position));
    }
    [[nodiscard]] RenderNode* scrollTarget(Point position) noexcept;
    [[nodiscard]] RenderNode* context_target(Point position) noexcept;
    bool openContextMenu(Point position, Size screen);
    bool scrollBy(int delta);
    bool handleScrollKey(const Event& event);
    [[nodiscard]] Rect scrollbarTrack() const;
    [[nodiscard]] Rect scrollbarThumb() const;
    bool scrollbarPress(Point position, int& grab_offset);
    bool scrollbar_drag(int y, int grab_offset);
    bool beginScrollbarDrag(Point position);
    bool moveScrollbarDrag(int y);
    void endScrollbarDrag() noexcept { scrollbar_drag_.end(); }
    [[nodiscard]] bool scrollable() const noexcept { return static_cast<bool>(on_scroll_); }
    [[nodiscard]] bool disabled() const noexcept { return disabled_; }
    [[nodiscard]] const std::function<void()>& onClick() const noexcept { return on_click_; }
    [[nodiscard]] bool checkable() const noexcept
    {
        return type_ == Component::Type::Checkbox || type_ == Component::Type::Switch ||
            type_ == Component::Type::Radio || type_ == Component::Type::RadioGroup;
    }
    [[nodiscard]] bool interactiveControl() const noexcept
    {
        return checkable() && !disabled_;
    }
    bool handleChoiceKey(const Event& event);
    bool handleChoiceClick(Point position);
    bool handleSliderKey(const Event& event);
    bool handleSliderMouse(Point position);
    [[nodiscard]] bool sliderInteractive() const noexcept
    {
        return type_ == Component::Type::Slider && !disabled_ && static_cast<bool>(slider_on_change_);
    }
    bool handleTabsKey(const Event& event);
    bool handleTabsClick(Point position);
    bool updateTabsHover(Point position);
    bool handleVirtualListKey(const Event& event);
    bool handleVirtualListClick(Point position);
    [[nodiscard]] int virtual_list_offset() const noexcept { return virtual_scroll_.offset(); }
    bool handleTableKey(const Event& event);
    bool handleTableClick(Point position);
    bool handleTreeKey(const Event& event);
    bool handleTreeClick(Point position);
    [[nodiscard]] Rect split_handle() const noexcept;
    [[nodiscard]] RenderNode* split_target(Point position) noexcept;
    bool handleSplitKey(const Event& event);
    bool dragSplit(Point position);
    struct TreeVisibleItem
    {
        const TreeItem* item;
        std::string parent;
        int depth;
        bool expanded;
        bool has_next = false;
        std::vector<bool> ancestor_has_next;
    }; /* End of struct TreeVisibleItem */
    [[nodiscard]] std::vector<TreeVisibleItem> tree_visible_items() const;
    [[nodiscard]] std::vector<std::size_t> table_order() const;
    [[nodiscard]] int table_sort_column() const noexcept { return table_sort_column_; }
    [[nodiscard]] bool table_sort_ascending() const noexcept { return table_sort_ascending_; }
    [[nodiscard]] bool tabPanelActive() const noexcept
    {
        return type_ != Component::Type::TabPanel || tab_panel_active_;
    }
    [[nodiscard]] bool editable() const noexcept { return static_cast<bool>(on_change_) && !disabled_; }
    bool handleTextInput(const Event& event);
    void setCursorFromX(int x);
    void setTextCursor(Point position);
    void paintTextArea(Painter& painter, std::string_view focused_id, const Theme* theme) const;
    bool scrollTextArea(int rows);
    [[nodiscard]] bool selectOpen() const noexcept
    {
        return (type_ == Component::Type::Select || type_ == Component::Type::Autocomplete ||
            type_ == Component::Type::DropdownMenu ||
            type_ == Component::Type::ContextMenu) && select_open_;
    }
    [[nodiscard]] Rect selectPopupRect(Size screen) const noexcept;
    [[nodiscard]] bool selectContains(Point point, Size screen) const noexcept;
    bool handleSelectKey(const Event& event, Size screen = {.width = 80,.height = 24});
    void updateAutocompleteOptions();
    [[nodiscard]] const std::string& autocompleteQuery() const noexcept { return autocomplete_query_; }
    [[nodiscard]] const std::vector<SelectOption>& autocompleteOptions() const noexcept { return select_options_; }
    bool handleSelectMouse(const MouseEvent& mouse, Size screen);
    [[nodiscard]] bool selectDragging() const noexcept { return select_open_ && select_drag_.active(); }
    [[nodiscard]] Rect select_scrollbar_track(Size screen) const noexcept;
    [[nodiscard]] Rect select_scrollbar_thumb(Size screen) const noexcept;
    void closeSelect() noexcept { select_open_ = false; select_drag_.end(); }
    void paintOverlay(Painter& painter, Size screen, const Theme& theme) const;
    [[nodiscard]] bool tooltipContains(Point point) const noexcept;
    [[nodiscard]] Rect tooltipRect(Point point, Size screen) const noexcept;
    void paintTooltip(Painter& painter, Point point, Size screen, const Theme& theme) const;
    [[nodiscard]] bool toast_open() const noexcept { return type_ == Component::Type::Toast && toast_visible_; }
    [[nodiscard]] Rect toastRect(Size screen, int bottom_offset = 0) const noexcept;
    void paintToast(Painter& painter, Size screen, const Theme& theme, int bottom_offset = 0) const;
    bool expireToast(std::chrono::steady_clock::time_point now);
    void closeToast();
    void paintOverlays(Painter& painter, Size screen, const Theme& theme) const;
    [[nodiscard]] bool dialog_open() const noexcept { return type_ == Component::Type::Dialog && dialog_open_; }
    [[nodiscard]] bool focus_scope_active() const noexcept
    {
        return type_ == Component::Type::FocusScope && focus_scope_active_;
    }
    [[nodiscard]] Rect dialogRect(Size screen) const;
    void layoutDialogChildren(Size screen);
    bool scrollDialog(int delta, Size screen);
    bool revealDialogFocus(std::uint64_t id, Size screen);
    [[nodiscard]] RenderNode* dialogChildAt(Point point) noexcept;
    bool handleDialogKey(const Event& event);
    bool handleDialogMouse(const MouseEvent& mouse, Size screen);
    void paintDialog(Painter& painter, Size screen, const Theme& theme,
        std::string_view focused_id = {}) const;

    [[nodiscard]] Size measure(const Constraints& constraints) const;
    void layout(const Constraints& constraints, Point origin = {});
    void paint(Painter& painter, std::string_view focused_id = {}, std::string_view hovered_id = {},
        std::optional<Color> background = std::nullopt, const Theme* theme = nullptr,
        bool unicode = true) const;
    [[nodiscard]] int height() const noexcept;
    [[nodiscard]] bool hasIndeterminateProgress() const noexcept;
    [[nodiscard]] bool layoutDirty() const noexcept { return layout_dirty_; }
    [[nodiscard]] bool paintDirty() const noexcept { return paint_dirty_; }

private:
    friend class Reconciler;

    RenderNode(std::uint64_t id, Component::Type type, std::string key) :
        id_(id), type_(type), key_(std::move(key))
    {}

    std::uint64_t id_;
    Component::Type type_;
    std::string key_;
    bool layout_dirty_ = true;
    bool paint_dirty_ = true;
    std::string text_;
    LayoutStyle style_;
    TextStyle text_style_;
    ColorVariant color_variant_ = ColorVariant::Default;
    std::optional<BorderStyle> border_;
    std::optional<Color> background_;
    ButtonVariant button_variant_ = ButtonVariant::Outlined;
    ButtonSize button_size_ = ButtonSize::Medium;
    CheckboxVariant checkbox_variant_ = CheckboxVariant::Default;
    TextFieldVariant text_field_variant_ = TextFieldVariant::Outlined;
    bool password_reveal_ = false;
    char32_t password_mask_ = U'•';
    double number_min_ = -1.0e100;
    double number_max_ = 1.0e100;
    double number_step_ = 1.0;
    double value_ = 0.0;
    ProgressVariant progress_variant_ = ProgressVariant::Blocks;
    bool progress_indeterminate_ = false;
    bool progress_show_percentage_ = false;
    double slider_min_ = 0.0;
    double slider_max_ = 100.0;
    double slider_step_ = 1.0;
    bool slider_show_value_ = false;
    std::function<void(double)> slider_on_change_;
    int columns_ = 1;
    SurfaceTone surface_tone_ = SurfaceTone::None;
    std::function<void()> on_click_;
    std::function<void(bool)> on_toggle_;
    std::function<void(ComponentEvent&)> on_event_capture_;
    std::function<void(ComponentEvent&)> on_event_;
    std::function<void(std::string)> on_change_;
    std::string placeholder_;
    std::size_t cursor_byte_ = 0;
    std::size_t selection_anchor_ = 0;
    bool selection_active_ = false;
    int text_area_min_rows_ = 4;
    bool read_only_ = false;
    mutable int text_area_first_row_ = 0;
    mutable int text_area_first_column_ = 0;
    int text_area_preferred_column_ = -1;
    mutable int scroll_columns_ = 0;
    ScrollController scroll_{};
    bool scrollbar_visible_ = true;
    std::function<void(int)> on_scroll_;
    bool disabled_ = false;
    std::vector<SelectOption> select_options_;
    std::vector<SelectOption> autocomplete_all_options_;
    std::string autocomplete_query_;
    std::string autocomplete_requested_query_;
    std::function<void(std::string)> autocomplete_on_input_change_;
    std::vector<RadioOption> radio_options_;
    std::vector<TabOption> tab_options_;
    std::vector<TableColumn> table_columns_;
    std::vector<TableRow> table_rows_;
    std::function<bool(const TableRow&)> table_filter_;
    std::function<TableCellFormat(const TableRow&,std::size_t)> table_format_cell_;
    ScrollController table_scroll_{};
    int table_sort_column_ = -1;
    bool table_sort_ascending_ = true;
    int virtual_item_count_ = 0;
    std::function<std::string(int)> virtual_item_text_;
    int virtual_selected_index_ = -1;
    std::function<void(int)> virtual_on_select_;
    ScrollController virtual_scroll_{};
    std::vector<TreeItem> tree_items_;
    std::vector<std::string> tree_expanded_;
    std::function<void(std::string,bool)> tree_on_toggle_;
    ScrollController tree_scroll_{};
    std::string tree_last_selected_;
    bool tree_pending_reveal_ = false;
    SplitDirection split_direction_ = SplitDirection::Horizontal;
    int split_position_ = 20;
    int split_min_first_ = 3;
    int split_min_second_ = 3;
    std::function<void(int)> split_on_change_;
    int hovered_tab_ = -1;
    bool tab_panel_active_ = true;
    bool checked_ = false;
    bool select_open_ = false;
    int select_highlight_ = 0;
    ScrollController select_scroll_{};
    ScrollController::DragState select_drag_{};
    Point context_anchor_{};
    bool dialog_open_ = false;
    bool toast_requested_ = false;
    bool toast_visible_ = false;
    int toast_duration_ms_ = 3500;
    int toast_severity_ = 0;
    std::function<void()> toast_on_close_;
    std::chrono::steady_clock::time_point toast_started_{};
    bool focus_scope_active_ = false;
    bool dialog_close_on_backdrop_ = true;
    bool dialog_show_actions_ = true;
    bool dialog_confirm_focused_ = false;
    int dialog_width_ = 48;
    std::string dialog_title_;
    std::string dialog_confirm_text_;
    std::string dialog_cancel_text_;
    std::function<void()> dialog_on_close_;
    std::uint64_t dialog_pressed_child_ = 0;
    ScrollController dialog_scroll_{};
    Rect dialog_content_rect_{};
    Size dialog_screen_{};
    ScrollController::DragState scrollbar_drag_{};
    [[nodiscard]] const ScrollController& scrollController() const noexcept;
    void applyScrollOffset(int target);
    Rect bounds_{};
    std::vector<std::unique_ptr<RenderNode>> children_;
    std::optional<Theme> theme_override_ = std::nullopt;
}; /* End of class RenderNode */

} /* End of namespace terion */

#endif /* TERION_RENDER_NODE_H */
