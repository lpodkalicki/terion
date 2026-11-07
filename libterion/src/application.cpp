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
        if (const auto* root = reconciler_.root()) root->paint(painter,focus_.current(),hovered_button_id_);
    }
    else if (paint_) paint_(painter,buffer.size());
    renderer_.render(buffer,*backend_);
}

void Application::syncFocusTargets()
{
    const std::string previous_focus(focus_.current());
    for (const auto& id : component_focus_ids_) focus_.remove(id);
    component_focus_ids_.clear();
    if (!component_) return;

    const auto visit = [&](const auto& self, RenderNode& node) -> void {
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
            component_focus_ids_.push_back(id);
        }
        else if (node.type() == Component::Type::TextField && node.editable())
        {
            const auto id = std::to_string(node.id());
            focus_.add(id,[&node](const Event& event) { return node.handleTextInput(event); });
            component_focus_ids_.push_back(id);
        }
        for (const auto& child : node.children()) self(self,*child);
    };
    if (auto* root = reconciler_.root()) visit(visit,*root);
    if (!previous_focus.empty()) focus_.focus(previous_focus);
    if (std::ranges::find(component_focus_ids_,hovered_button_id_) == component_focus_ids_.end()) hovered_button_id_.clear();
    if (std::ranges::find(component_focus_ids_,pressed_button_id_) == component_focus_ids_.end()) pressed_button_id_.clear();
}

bool Application::handleMouse(const MouseEvent& mouse)
{
    RenderNode* hit = nullptr;
    if (component_ && reconciler_.root()) hit = reconciler_.root()->hitTest(mouse.position);
    if (hit && !((hit->type() == Component::Type::Button && !hit->disabled() && hit->onClick()) ||
        (hit->type() == Component::Type::TextField && hit->editable()))) hit = nullptr;
    const std::string target = hit ? std::to_string(hit->id()) : std::string{};

    const std::string hovered = hit && hit->type() == Component::Type::Button ? target : std::string{};
    if (hovered_button_id_ != hovered)
    {
        hovered_button_id_ = hovered;
        invalidate();
    }
    if (mouse.action == MouseAction::Press && mouse.button == MouseButton::Left)
    {
        pressed_button_id_ = target;
        if (hit) focus_.focus(target);
        if (hit && hit->type() == Component::Type::TextField)
        {
            hit->setCursorFromX(mouse.position.x);
            pressed_button_id_.clear();
        }
        return hit != nullptr;
    }
    if (mouse.action == MouseAction::Release &&
        (mouse.button == MouseButton::Left || mouse.button == MouseButton::None))
    {
        const bool activate = hit && hit->type() == Component::Type::Button &&
            !pressed_button_id_.empty() && pressed_button_id_ == target;
        pressed_button_id_.clear();
        if (activate) { hit->onClick()(); return true; }
    }
    if (mouse.action == MouseAction::Scroll) return false;
    return hit != nullptr;
}

bool Application::dispatchEvent(const Event& event)
{
    if (event.key == Key::Resize) renderer_.invalidate();
    invalidate();
    if (event.key == Key::Mouse && handleMouse(event.mouse)) return true;
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

