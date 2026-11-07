/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <chrono>
#include <stdexcept>
#include <thread>
#include <algorithm>
#include <terion/application.h>

using namespace terion;

Application::Application(std::unique_ptr<TerminalBackend> backend) :
    backend_(std::move(backend)),
    tracker_([weak = std::weak_ptr<std::atomic_bool>(dirty_)] {
        if (const auto flag = weak.lock()) flag->store(true);
    })
{
    if (!backend_) throw std::invalid_argument("backend is null");
}

void Application::renderOnce()
{
    dirty_->store(false);
    ScreenBuffer buffer(backend_->size());
    Painter painter(buffer);
    DependencyTracker::Scope tracking(tracker_);
    if (component_)
    {
        reconciler_.update(component_());
        if (auto* root = reconciler_.root())
        {
            root->layout(Constraints{.maximum = buffer.size()});
        }
    }
    syncFocusTargets();
    if (component_)
    {
        if (const auto* root = reconciler_.root()) root->paint(painter,focus_.current());
    }
    else if (paint_) paint_(painter,buffer.size());
    renderer_.render(buffer,*backend_);
}

void Application::syncFocusTargets()
{
    const std::string previous_focus(focus_.current());
    for (const auto& id : button_focus_ids_) focus_.remove(id);
    button_focus_ids_.clear();
    if (!component_) return;

    const auto visit = [&](const auto& self, const RenderNode& node) -> void {
        if (node.type() == Component::Type::Button && !node.disabled() && node.onClick())
        {
            const auto id = std::to_string(node.id());
            const auto callback = node.onClick();
            focus_.add(id,[callback](const Event& event) {
                if (event.key != Key::Enter &&
                    !(event.key == Key::Character && event.character == U' ')) return false;
                callback();
                return true;
            });
            button_focus_ids_.push_back(id);
        }
        for (const auto& child : node.children()) self(self,*child);
    };
    if (const auto* root = reconciler_.root()) visit(visit,*root);
    if (!previous_focus.empty()) focus_.focus(previous_focus);
}

bool Application::dispatchEvent(const Event& event)
{
    if (event.key == Key::Resize) renderer_.invalidate();
    invalidate();
    if (focus_.dispatch(event)) return true;
    if (on_event_ && on_event_(event)) return true;
    if (event.key == Key::Escape)
    {
        quit();
        return true;
    }
    return false;
}

int Application::run()
{
    running_ = true;
    Size lastSize {.width = -1,.height = -1};
    while (running_)
    {
        const auto size = backend_->size();
        if (size != lastSize) { renderer_.invalidate(); lastSize = size; invalidate(); }
        if (needsRender()) renderOnce();
        // Drain a bounded number of pending events to avoid delaying input under load.
        for (int i = 0; i < 64 && running_; ++i)
        {
            const auto event = backend_->pollEvent();
            if (!event) break;
            dispatchEvent(*event);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return 0;
}

