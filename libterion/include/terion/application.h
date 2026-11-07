/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#ifndef TERION_APPLICATION_H
#define TERION_APPLICATION_H

#include <functional>
#include <memory>
#include <atomic>
#include <terion/component.h>
#include <terion/focus.h>
#include <terion/reconciler.h>
#include <terion/state.h>
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
        invalidate();
    }

    void render(ComponentCallback callback)
    {
        component_ = std::move(callback);
        paint_ = {};
        invalidate();
    }

    void renderOnce();
    void invalidate() noexcept { dirty_->store(true); }
    [[nodiscard]] bool needsRender() const noexcept { return dirty_->load(); }
    void onEvent(EventCallback callback) { on_event_ = std::move(callback); }
    bool dispatchEvent(const Event& event);
    int run();

    [[nodiscard]] FocusManager& focus() noexcept { return focus_; }
    [[nodiscard]] const Reconciler& reconciler() const noexcept { return reconciler_; }

    void quit() noexcept
    {
        running_ = false;
    }

    [[nodiscard]] TerminalBackend& backend() const
    {
        return *backend_;
    }

private:
    std::unique_ptr<TerminalBackend> backend_;
    DifferentialRenderer renderer_;
    PaintCallback paint_;
    ComponentCallback component_;
    Reconciler reconciler_;
    std::shared_ptr<std::atomic_bool> dirty_ = std::make_shared<std::atomic_bool>(true);
    DependencyTracker tracker_;
    EventCallback on_event_;
    FocusManager focus_;
    bool running_ = false;
}; /* End of class Application */

} /* End of namespace terion */

#endif /* TERION_APPLICATION_H */
