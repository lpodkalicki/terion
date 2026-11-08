/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#ifndef TERION_APPLICATION_H
#define TERION_APPLICATION_H

#include <functional>
#include <memory>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <vector>
#include <terion/component.h>
#include <terion/focus.h>
#include <terion/reconciler.h>
#include <terion/overlay.h>
#include <terion/toast_manager.h>
#include <terion/state.h>
#include <terion/theme.h>
#include <terion/terminal.h>
#include <terion/render/painter.h>
#include <terion/render/screen_buffer.h>
#include <terion/render/differential_renderer.h>

namespace terion {

class Application
{
public:
    using PaintCallback = std::function<void(Painter&, Size)>;
    using ComponentCallback = std::function<Component()>;
    using EventCallback = std::function<bool(const Event&)>;

    explicit Application(std::unique_ptr<TerminalBackend> backend);

    void render(PaintCallback callback)
    {
        paint_ = std::move(callback);
        component_ = {};
        base_frame_cache_.reset();
        invalidate();
    }

    void render(ComponentCallback callback)
    {
        component_ = std::move(callback);
        paint_ = {};
        base_frame_cache_.reset();
        invalidate();
    }

    void renderOnce();
    void invalidate() noexcept { dirty_->store(true); }
    [[nodiscard]] bool needsRender() const noexcept { return dirty_->load(); }
    void onEvent(EventCallback callback) { on_event_ = std::move(callback); }
    std::uint64_t addShortcut(Shortcut shortcut, std::function<void()> callback);
    bool removeShortcut(std::uint64_t id);
    bool dispatchEvent(const Event& event);
    int run();

    [[nodiscard]] FocusManager& focus() noexcept { return focus_; }
    [[nodiscard]] ToastManager& toasts() noexcept { return toasts_; }
    bool focus(std::string_view component_key);
    void blur() noexcept { focus_.blur(); invalidate(); }
    [[nodiscard]] std::string_view focused() const noexcept { return focus_.current(); }
    [[nodiscard]] bool hasFocus(std::string_view component_key) const;
    void setTheme(Theme theme) { theme_ = std::move(theme); base_frame_cache_.reset(); invalidate(); }
    [[nodiscard]] const Theme& theme() const noexcept { return theme_; }
    [[nodiscard]] const Reconciler& reconciler() const noexcept { return reconciler_; }
    struct RenderStats
    {
        std::uint64_t base_paints = 0;
        std::uint64_t cache_hits = 0;
    }; /* End of struct RenderStats */
    [[nodiscard]] RenderStats renderStats() const noexcept { return render_stats_; }
    struct FrameTimings
    {
        std::chrono::nanoseconds build{};
        std::chrono::nanoseconds reconcile{};
        std::chrono::nanoseconds layout{};
        std::chrono::nanoseconds paint{};
        std::chrono::nanoseconds terminal{};
        std::chrono::nanoseconds total{};
    }; /* End of struct FrameTimings */
    [[nodiscard]] FrameTimings lastFrameTimings() const noexcept { return frame_timings_; }

    void quit() noexcept
    {
        running_ = false;
    }

    [[nodiscard]] TerminalBackend& backend() const
    {
        return *backend_;
    }

private:
    void syncFocusTargets();
    bool handleMouse(const MouseEvent& mouse);
    void updateHover(const MouseEvent& mouse);
    void updateMousePointer(const MouseEvent& mouse);

    std::unique_ptr<TerminalBackend> backend_;
    DifferentialRenderer renderer_;
    PaintCallback paint_;
    ComponentCallback component_;
    Reconciler reconciler_;
    OverlayManager overlays_;
    std::shared_ptr<std::atomic_bool> dirty_ = std::make_shared<std::atomic_bool>(true);
    DependencyTracker tracker_;
    ToastManager toasts_{[this] { invalidate(); }};
    EventCallback on_event_;
    struct ShortcutEntry
    {
        std::uint64_t id;
        Shortcut shortcut;
        std::function<void()> callback;
    };
    std::vector<ShortcutEntry> shortcuts_;
    std::uint64_t next_shortcut_id_ = 1;
    FocusManager focus_;
    Theme theme_;
    std::vector<std::string> component_focus_ids_;
    std::string background_focus_id_;
    std::string hovered_component_id_;
    std::string pressed_button_id_;
    std::uint64_t scrollbar_drag_id_ = 0;
    std::uint64_t split_drag_id_ = 0;
    std::uint64_t slider_drag_id_ = 0;
    int scrollbar_grab_offset_ = 0;
    MousePointerShape pointer_shape_ = MousePointerShape::Default;
    bool pointer_shape_initialized_ = false;
    Size last_component_layout_size_{.width = -1,.height = -1};
    std::optional<ScreenBuffer> base_frame_cache_;
    std::string cached_focus_id_;
    std::string cached_hover_id_;
    bool cached_unicode_ = true;
    RenderStats render_stats_{};
    FrameTimings frame_timings_{};
    bool running_ = false;
}; /* End of class Application */

} /* End of namespace terion */

#endif /* TERION_APPLICATION_H */
