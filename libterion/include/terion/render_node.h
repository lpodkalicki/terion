/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#ifndef TERION_RENDER_NODE_H
#define TERION_RENDER_NODE_H

#include <cstdint>
#include <memory>
#include <string>
#include <functional>
#include <string_view>
#include <vector>
#include <terion/component.h>
#include <terion/layout.h>
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
    bool scrollBy(int delta);
    bool handleScrollKey(const Event& event);
    [[nodiscard]] Rect scrollbarTrack() const noexcept;
    [[nodiscard]] Rect scrollbarThumb() const noexcept;
    bool scrollbarPress(Point position, int& grab_offset);
    bool scrollbarDrag(int y, int grab_offset);
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
    [[nodiscard]] bool editable() const noexcept { return static_cast<bool>(on_change_) && !disabled_; }
    bool handleTextInput(const Event& event);
    void setCursorFromX(int x);
    [[nodiscard]] bool selectOpen() const noexcept { return type_ == Component::Type::Select && select_open_; }
    [[nodiscard]] Rect selectPopupRect(Size screen) const noexcept;
    [[nodiscard]] bool selectContains(Point point, Size screen) const noexcept;
    bool handleSelectKey(const Event& event, Size screen = {.width = 80,.height = 24});
    bool handleSelectMouse(const MouseEvent& mouse, Size screen);
    [[nodiscard]] bool selectDragging() const noexcept { return select_open_ && select_dragging_; }
    [[nodiscard]] Rect selectScrollbarTrack(Size screen) const noexcept;
    [[nodiscard]] Rect selectScrollbarThumb(Size screen) const noexcept;
    void closeSelect() noexcept { select_open_ = false; select_dragging_ = false; }
    void paintOverlay(Painter& painter, Size screen, const Theme& theme) const;
    void paintOverlays(Painter& painter, Size screen, const Theme& theme) const;
    [[nodiscard]] bool dialogOpen() const noexcept { return type_ == Component::Type::Dialog && dialog_open_; }
    [[nodiscard]] bool focusScopeActive() const noexcept
    {
        return type_ == Component::Type::FocusScope && focus_scope_active_;
    }
    [[nodiscard]] Rect dialogRect(Size screen) const;
    void layoutDialogChildren(Size screen);
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

private:
    friend class Reconciler;

    RenderNode(std::uint64_t id, Component::Type type, std::string key) :
        id_(id), type_(type), key_(std::move(key))
    {}

    std::uint64_t id_;
    Component::Type type_;
    std::string key_;
    std::string text_;
    LayoutStyle style_;
    TextStyle text_style_;
    std::optional<BorderStyle> border_;
    std::optional<Color> background_;
    ButtonVariant button_variant_ = ButtonVariant::Outlined;
    ButtonSize button_size_ = ButtonSize::Medium;
    CheckboxVariant checkbox_variant_ = CheckboxVariant::Default;
    TextFieldVariant text_field_variant_ = TextFieldVariant::Outlined;
    double value_ = 0.0;
    int columns_ = 1;
    SurfaceTone surface_tone_ = SurfaceTone::None;
    std::function<void()> on_click_;
    std::function<void(bool)> on_toggle_;
    std::function<void(ComponentEvent&)> on_event_capture_;
    std::function<void(ComponentEvent&)> on_event_;
    std::function<void(std::string)> on_change_;
    std::string placeholder_;
    std::size_t cursor_byte_ = 0;
    mutable int scroll_columns_ = 0;
    int scroll_offset_ = 0;
    int max_scroll_offset_ = 0;
    int viewport_height_ = 0;
    bool scrollbar_visible_ = true;
    std::function<void(int)> on_scroll_;
    bool disabled_ = false;
    std::vector<SelectOption> select_options_;
    std::vector<RadioOption> radio_options_;
    bool checked_ = false;
    bool select_open_ = false;
    int select_highlight_ = 0;
    int select_scroll_ = 0;
    bool select_dragging_ = false;
    int select_grab_offset_ = 0;
    bool dialog_open_ = false;
    bool focus_scope_active_ = false;
    bool dialog_close_on_backdrop_ = true;
    bool dialog_confirm_focused_ = false;
    int dialog_width_ = 48;
    std::string dialog_title_;
    std::string dialog_confirm_text_;
    std::string dialog_cancel_text_;
    std::function<void()> dialog_on_close_;
    std::uint64_t dialog_pressed_child_ = 0;
    Rect bounds_{};
    std::vector<std::unique_ptr<RenderNode>> children_;
}; /* End of class RenderNode */

} /* End of namespace terion */

#endif /* TERION_RENDER_NODE_H */
