/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <chrono>
#include <stdexcept>
#include <thread>
#include <algorithm>
#include <charconv>
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
    painter.fillBackground({.width = buffer.size().width,.height = buffer.size().height},theme_.palette.background);
    DependencyTracker::Scope tracking(tracker_);
    if (component_)
    {
        reconciler_.update(component_());
        if (auto* root = reconciler_.root())
        {
            root->layout(Constraints{.maximum = buffer.size()});
        }
        overlays_.collect(reconciler_.root());
        if (auto* dialog = overlays_.activeDialog()) dialog->layoutDialogChildren(buffer.size());
    }
    syncFocusTargets();
    if (component_)
    {
        if (const auto* root = reconciler_.root())
        {
            root->paint(painter,focus_.current(),hovered_component_id_,std::nullopt,&theme_,
                backend_->capabilities().unicode);
            overlays_.paint(painter,buffer.size(),theme_,focus_.current());
        }
    }
    else if (paint_) paint_(painter,buffer.size());
    renderer_.render(buffer,*backend_);
}

void Application::syncFocusTargets()
{
    const std::string previous_focus(focus_.current());
    const std::string previous_scope(focus_.activeScope());
    const bool previous_targets = !component_focus_ids_.empty();
    const auto* dialog = overlays_.activeDialog();
    std::string next_scope = dialog ? std::to_string(dialog->id()) : std::string{};
    if (!dialog && reconciler_.root())
    {
        const auto find_active = [&](const auto& self, const RenderNode& node) -> const RenderNode* {
            if (!node.tabPanelActive()) return nullptr;
            if (node.type() == Component::Type::Dialog && !node.dialogOpen()) return nullptr;
            if (node.focusScopeActive()) return &node;
            for (const auto& child : node.children())
                if (const auto* found = self(self,*child)) return found;
            return nullptr;
        };
        if (const auto* active = find_active(find_active,*reconciler_.root()))
            next_scope = std::to_string(active->id());
    }
    if (previous_scope.empty() && !next_scope.empty()) background_focus_id_ = previous_focus;
    for (const auto& id : component_focus_ids_) focus_.remove(id);
    component_focus_ids_.clear();
    if (!component_) return;

    const auto visit = [&](const auto& self, RenderNode& node, const std::string& scope) -> void {
        if (!node.tabPanelActive()) return;
        if (node.type() == Component::Type::Dialog)
        {
            if (!node.dialogOpen()) return;
            const std::string child_scope = std::to_string(node.id());
            for (const auto& child : node.children()) self(self,*child,child_scope);
            return;
        }
        if (node.type() == Component::Type::FocusScope)
        {
            const std::string child_scope = scope.empty() && node.focusScopeActive() ?
                std::to_string(node.id()) : scope;
            for (const auto& child : node.children()) self(self,*child,child_scope);
            return;
        }
        if (node.type() == Component::Type::Button && !node.disabled() && node.onClick())
        {
            const auto id = std::to_string(node.id());
            const auto callback = node.onClick();
            focus_.add(id,[callback](const Event& event) {
                if (event.key != Key::Enter &&
                    !(event.key == Key::Character && event.character == U' ')) return false;
                callback();
                return true;
            },scope);
            component_focus_ids_.push_back(id);
        }
        else if (node.type() == Component::Type::TextField && node.editable())
        {
            const auto id = std::to_string(node.id());
            focus_.add(id,[&node](const Event& event) { return node.handleTextInput(event); },scope);
            component_focus_ids_.push_back(id);
        }
        else if (node.type() == Component::Type::ScrollView && node.scrollable())
        {
            const auto id = std::to_string(node.id());
            focus_.add(id,[&node](const Event& event) { return node.handleScrollKey(event); },scope);
            component_focus_ids_.push_back(id);
        }
        else if ((node.type() == Component::Type::Select || node.type() == Component::Type::DropdownMenu) &&
            !node.disabled())
        {
            const auto id = std::to_string(node.id());
            focus_.add(id,[this,&node](const Event& event) { return node.handleSelectKey(event,backend_->size()); },scope);
            component_focus_ids_.push_back(id);
        }
        else if (node.interactiveControl())
        {
            const auto id = std::to_string(node.id());
            focus_.add(id,[&node](const Event& event) { return node.handleChoiceKey(event); },scope);
            component_focus_ids_.push_back(id);
        }
        else if (node.type() == Component::Type::Tabs && !node.disabled())
        {
            const auto id = std::to_string(node.id());
            focus_.add(id,[&node](const Event& event) { return node.handleTabsKey(event); },scope);
            component_focus_ids_.push_back(id);
        }
        else if (node.type() == Component::Type::Table && !node.disabled())
        {
            const auto id = std::to_string(node.id());
            focus_.add(id,[&node](const Event& event) { return node.handleTableKey(event); },scope);
            component_focus_ids_.push_back(id);
        }
        else if (node.type() == Component::Type::TreeView && !node.disabled())
        {
            const auto id = std::to_string(node.id());
            focus_.add(id,[&node](const Event& event) { return node.handleTreeKey(event); },scope);
            component_focus_ids_.push_back(id);
        }
        for (const auto& child : node.children()) self(self,*child,scope);
    };
    if (auto* root = reconciler_.root()) visit(visit,*root,{});
    focus_.setActiveScope(next_scope);
    if (previous_scope == next_scope && !previous_focus.empty()) focus_.focus(previous_focus);
    if (previous_scope == next_scope && previous_focus.empty() && previous_targets) focus_.blur();
    if (!previous_scope.empty() && next_scope.empty() && !background_focus_id_.empty())
    {
        focus_.focus(background_focus_id_);
        background_focus_id_.clear();
    }
    if (std::ranges::find(component_focus_ids_,hovered_component_id_) == component_focus_ids_.end()) hovered_component_id_.clear();
    if (std::ranges::find(component_focus_ids_,pressed_button_id_) == component_focus_ids_.end()) pressed_button_id_.clear();
}

bool Application::handleMouse(const MouseEvent& mouse)
{
    if (auto* dialog = overlays_.activeDialog())
    {
        if (mouse.action == MouseAction::Press)
            if (auto* child = dialog->dialogChildAt(mouse.position))
                focus_.focus(std::to_string(child->id()));
        (void)overlays_.handleMouse(mouse,backend_->size());
        return true;
    }
    updateHover(mouse);
    updateMousePointer(mouse);
    if (component_ && reconciler_.root())
    {
        if (auto* target = overlays_.handleMouse(mouse,backend_->size()))
        {
            overlays_.dismissExcept(target);
            focus_.focus(std::to_string(target->id()));
            return true;
        }
        overlays_.dismissOutside(mouse,backend_->size());
    }
    if (scrollbar_drag_id_ != 0)
    {
        if (mouse.action == MouseAction::Release)
        {
            scrollbar_drag_id_ = 0;
            updateHover(mouse);
            updateMousePointer(mouse);
            return true;
        }
        if (mouse.action == MouseAction::Move)
        {
            const auto find = [&](const auto& self, RenderNode& node) -> RenderNode* {
                if (node.id() == scrollbar_drag_id_) return &node;
                for (const auto& child : node.children())
                    if (auto* target = self(self,*child)) return target;
                return nullptr;
            };
            if (component_ && reconciler_.root())
                if (auto* target = find(find,*reconciler_.root()))
                    return target->scrollbarDrag(mouse.position.y,scrollbar_grab_offset_);
            scrollbar_drag_id_ = 0;
        }
    }
    if (mouse.action == MouseAction::Scroll && component_ && reconciler_.root())
    {
        if (auto* target = reconciler_.root()->scrollTarget(mouse.position))
            return target->scrollBy(-mouse.scrollY * 3);
    }
    if (mouse.action == MouseAction::Press && mouse.button == MouseButton::Left &&
        component_ && reconciler_.root())
    {
        if (auto* target = reconciler_.root()->scrollTarget(mouse.position))
        {
            int grab_offset = 0;
            if (target->scrollbarPress(mouse.position,grab_offset))
            {
                scrollbar_drag_id_ = target->id();
                scrollbar_grab_offset_ = grab_offset;
                updateHover(mouse);
                updateMousePointer(mouse);
                pressed_button_id_.clear();
                focus_.focus(std::to_string(target->id()));
                return true;
            }
        }
    }
    RenderNode* hit = nullptr;
    if (component_ && reconciler_.root()) hit = reconciler_.root()->hitTest(mouse.position);
    if (hit && !(((hit->type() == Component::Type::Select ||
        hit->type() == Component::Type::DropdownMenu) && !hit->disabled()) ||
        (hit->type() == Component::Type::Button && !hit->disabled() && hit->onClick()) ||
        (hit->type() == Component::Type::TextField && hit->editable()) ||
        (hit->type() == Component::Type::Tabs && !hit->disabled()) ||
        ((hit->type() == Component::Type::Table || hit->type() == Component::Type::TreeView) &&
            !hit->disabled()) ||
        hit->interactiveControl())) hit = nullptr;
    const std::string target = hit ? std::to_string(hit->id()) : std::string{};

    if (mouse.action == MouseAction::Press && mouse.button == MouseButton::Left)
    {
        pressed_button_id_ = target;
        if (hit) focus_.focus(target);
        else if (focus_.activeScope().empty()) focus_.blur();
        if (hit && (hit->type() == Component::Type::Select || hit->type() == Component::Type::DropdownMenu))
        {
            overlays_.dismissExcept(hit);
            return hit->handleSelectMouse(mouse,backend_->size());
        }
        if (!hit && component_ && reconciler_.root())
        {
            if (auto* scroll = reconciler_.root()->scrollTarget(mouse.position))
            {
                focus_.focus(std::to_string(scroll->id()));
                pressed_button_id_.clear();
                return true;
            }
        }
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
        const bool activate = hit && (hit->type() == Component::Type::Button ||
            hit->type() == Component::Type::Tabs || hit->type() == Component::Type::Table ||
            hit->type() == Component::Type::TreeView ||
            hit->interactiveControl()) &&
            !pressed_button_id_.empty() && pressed_button_id_ == target;
        pressed_button_id_.clear();
        if (activate)
        {
            if (hit->interactiveControl()) return hit->handleChoiceClick(mouse.position);
            if (hit->type() == Component::Type::Tabs) return hit->handleTabsClick(mouse.position);
            if (hit->type() == Component::Type::Table) return hit->handleTableClick(mouse.position);
            if (hit->type() == Component::Type::TreeView) return hit->handleTreeClick(mouse.position);
            if (hit->onClick()) hit->onClick()();
            return true;
        }
    }
    if (mouse.action == MouseAction::Scroll) return false;
    return hit != nullptr;
}

void Application::updateHover(const MouseEvent& mouse)
{
    std::string hovered;
    if (scrollbar_drag_id_ != 0)
        hovered = std::to_string(scrollbar_drag_id_);
    else if (component_ && reconciler_.root())
    {
        auto* root = reconciler_.root();
        if (auto* scroll = root->scrollTarget(mouse.position))
        {
            const Rect track = scroll->scrollbarTrack();
            if (mouse.position.x >= track.x && mouse.position.x < track.x + track.width &&
                mouse.position.y >= track.y && mouse.position.y < track.y + track.height)
                hovered = std::to_string(scroll->id());
        }
        if (hovered.empty())
        {
            if (auto* hit = root->hitTest(mouse.position))
            {
                if (hit->type() == Component::Type::Tabs && hit->updateTabsHover(mouse.position))
                    invalidate();
                if (((hit->type() == Component::Type::Select ||
                    hit->type() == Component::Type::DropdownMenu) && !hit->disabled()) ||
                    (hit->type() == Component::Type::Button && !hit->disabled() && hit->onClick()) ||
                    (hit->type() == Component::Type::TextField && hit->editable()) ||
                    hit->interactiveControl() || (hit->type() == Component::Type::Tabs && !hit->disabled()))
                    hovered = std::to_string(hit->id());
            }
        }
    }
    if (hovered_component_id_ != hovered)
    {
        hovered_component_id_ = std::move(hovered);
        invalidate();
    }
}

void Application::updateMousePointer(const MouseEvent& mouse)
{
    if (!backend_->capabilities().mousePointerShape) return;
    MousePointerShape shape = MousePointerShape::Default;
    if (scrollbar_drag_id_ != 0 && mouse.action != MouseAction::Release)
        shape = MousePointerShape::Grabbing;
    else if (component_ && reconciler_.root())
    {
        RenderNode* root = reconciler_.root();
        if (auto* scroll = root->scrollTarget(mouse.position))
        {
            const Rect track = scroll->scrollbarTrack();
            if (track.width > 0 && mouse.position.x >= track.x &&
                mouse.position.x < track.x + track.width && mouse.position.y >= track.y &&
                mouse.position.y < track.y + track.height)
                shape = MousePointerShape::Grab;
        }
        if (shape == MousePointerShape::Default)
        {
            if (const auto* hit = root->hitTest(mouse.position))
            {
                if (hit->type() == Component::Type::TextField || hit->type() == Component::Type::Text)
                    shape = MousePointerShape::Text;
                else if ((hit->type() == Component::Type::Button && !hit->disabled() && hit->onClick()) ||
                    ((hit->type() == Component::Type::Select ||
                        hit->type() == Component::Type::DropdownMenu) && !hit->disabled()) ||
                    hit->interactiveControl() ||
                    (hit->type() == Component::Type::Tabs && !hit->disabled()))
                    shape = MousePointerShape::Pointer;
            }
        }
    }
    if (!pointer_shape_initialized_ || pointer_shape_ != shape)
    {
        backend_->setMousePointerShape(shape);
        pointer_shape_ = shape;
        pointer_shape_initialized_ = true;
    }
}

bool Application::dispatchEvent(const Event& event)
{
    if (event.key == Key::Resize) renderer_.invalidate();
    invalidate();
    if (component_ && reconciler_.root() && event.key != Key::Resize)
    {
        auto* root = reconciler_.root();
        std::uint64_t target = root->id();
        if (auto* dialog = overlays_.activeDialog())
        {
            target = dialog->id();
            if (event.key == Key::Mouse)
                if (auto* child = dialog->dialogChildAt(event.mouse.position)) target = child->id();
            if (event.key != Key::Mouse && !focus_.current().empty())
            {
                std::uint64_t id = 0;
                const auto focused = focus_.current();
                const auto [end, error] = std::from_chars(focused.data(),focused.data()+focused.size(),id);
                if (error == std::errc{} && end == focused.data()+focused.size()) target = id;
            }
        }
        else if (event.key == Key::Mouse)
        {
            if (auto* hit = root->hitTest(event.mouse.position)) target = hit->id();
        }
        else if (!focus_.current().empty())
        {
            std::uint64_t id = 0;
            const auto focused = focus_.current();
            const auto [end, error] = std::from_chars(focused.data(),focused.data()+focused.size(),id);
            if (error == std::errc{} && end == focused.data()+focused.size()) target = id;
        }
        if (root->propagateEvent(event,target)) return true;
    }
    if (auto* dialog = overlays_.activeDialog(); dialog && event.key != Key::Resize && event.key != Key::Mouse)
    {
        if (event.key == Key::Escape || !focus_.hasTargetsInActiveScope()) return dialog->handleDialogKey(event);
        if (focus_.dispatch(event)) return true;
        return true;
    }
    if (event.key != Key::Resize && event.key != Key::Mouse && overlays_.handleKey(event)) return true;
    if (event.key == Key::Mouse && handleMouse(event.mouse)) return true;
    if (focus_.dispatch(event))
    {
        if (reconciler_.root())
        {
            const auto find = [&](const auto& self, RenderNode& node) -> RenderNode* {
                if ((node.type() == Component::Type::Select || node.type() == Component::Type::DropdownMenu) &&
                    node.selectOpen() &&
                    std::to_string(node.id()) == focus_.current()) return &node;
                for (const auto& child : node.children())
                    if (auto* selected = self(self,*child)) return selected;
                return nullptr;
            };
            if (auto* selected = find(find,*reconciler_.root())) overlays_.dismissExcept(selected);
        }
        return true;
    }
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

