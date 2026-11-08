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

bool Application::focus(const std::string_view component_key)
{
    if (!reconciler_.root() || component_key.empty()) return false;
    const auto find = [&](const auto& self, const RenderNode& node) -> const RenderNode* {
        if (node.key() == component_key) return &node;
        for (const auto& child : node.children())
            if (const auto* found = self(self,*child)) return found;
        return nullptr;
    };
    const auto* node = find(find,*reconciler_.root());
    if (!node || !focus_.focus(std::to_string(node->id()))) return false;
    invalidate();
    return true;
}

bool Application::hasFocus(const std::string_view component_key) const
{
    if (!reconciler_.root() || focus_.current().empty()) return false;
    const auto find = [&](const auto& self, const RenderNode& node) -> bool {
        if (node.key() == component_key && focus_.current() == std::to_string(node.id())) return true;
        for (const auto& child : node.children()) if (self(self,*child)) return true;
        return false;
    };
    return find(find,*reconciler_.root());
}

std::uint64_t Application::addShortcut(const Shortcut shortcut, std::function<void()> callback)
{
    if (!callback || shortcut.key == Key::Unknown || shortcut.key == Key::Mouse ||
        shortcut.key == Key::Resize)
        throw std::invalid_argument("shortcut requires a keyboard key and callback");
    const auto id = next_shortcut_id_++;
    shortcuts_.push_back({.id = id,.shortcut = shortcut,.callback = std::move(callback)});
    return id;
}

bool Application::removeShortcut(const std::uint64_t id)
{
    const auto old_size = shortcuts_.size();
    std::erase_if(shortcuts_,[id](const ShortcutEntry& item) { return item.id == id; });
    return old_size != shortcuts_.size();
}

void Application::renderOnce()
{
    using Clock = std::chrono::steady_clock;
    const auto frame_start = Clock::now();
    FrameTimings timings{};
    dirty_->store(false);
    ScreenBuffer buffer(backend_->size());
    DependencyTracker::Scope tracking(tracker_);
    if (component_)
    {
        const auto build_start = Clock::now();
        auto tree = component_();
        const auto build_end = Clock::now();
        timings.build = build_end-build_start;
        reconciler_.update(tree);
        const auto reconcile_end = Clock::now();
        timings.reconcile = reconcile_end-build_end;
        if (auto* root = reconciler_.root())
        {
            if (root->layoutDirty() || last_component_layout_size_ != buffer.size())
            {
                root->layout(Constraints{.maximum = buffer.size()});
                last_component_layout_size_ = buffer.size();
            }
        }
        overlays_.collect(reconciler_.root());
        if (auto* dialog = overlays_.activeDialog()) dialog->layoutDialogChildren(buffer.size());
        timings.layout = Clock::now()-reconcile_end;
    }
    const auto paint_start = Clock::now();
    syncFocusTargets();
    const bool unicode = backend_->capabilities().unicode;
    const bool reuse_base = component_ && reconciler_.root() && base_frame_cache_ &&
        base_frame_cache_->size() == buffer.size() &&
        !reconciler_.root()->paintDirty() &&
        !reconciler_.root()->hasIndeterminateProgress() &&
        cached_focus_id_ == focus_.current() && cached_hover_id_ == hovered_component_id_ &&
        cached_unicode_ == unicode;
    if (reuse_base)
    {
        buffer = *base_frame_cache_;
        ++render_stats_.cache_hits;
    }
    Painter painter(buffer);
    if (!reuse_base)
    {
        painter.fillBackground({.width = buffer.size().width,.height = buffer.size().height},theme_.palette.background);
        if (component_)
        {
            if (const auto* root = reconciler_.root())
            {
                root->paint(painter,focus_.current(),hovered_component_id_,std::nullopt,&theme_,unicode);
                base_frame_cache_ = buffer;
                cached_focus_id_ = focus_.current();
                cached_hover_id_ = hovered_component_id_;
                cached_unicode_ = unicode;
                ++render_stats_.base_paints;
            }
        }
        else if (paint_) paint_(painter,buffer.size());
    }
    if (component_)
    {
        if (reconciler_.root())
            overlays_.paint(painter,buffer.size(),theme_,focus_.current());
    }
    if (!overlays_.modalOpen())
        toasts_.paint(painter,buffer.size(),theme_,overlays_.toastOccupiedHeight(buffer.size()));
    timings.paint = Clock::now()-paint_start;
    const auto terminal_start = Clock::now();
    renderer_.render(buffer,*backend_);
    const auto frame_end = Clock::now();
    timings.terminal = frame_end-terminal_start;
    timings.total = frame_end-frame_start;
    frame_timings_ = timings;
    if (component_) reconciler_.commitEffects();
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
        else if (node.sliderInteractive())
        {
            const auto id = std::to_string(node.id());
            focus_.add(id,[&node](const Event& event) { return node.handleSliderKey(event); },scope);
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
        else if (node.type() == Component::Type::ResizableSplit)
        {
            const auto id = std::to_string(node.id());
            focus_.add(id,[&node](const Event& event) { return node.handleSplitKey(event); },scope);
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
    overlays_.setHover(mouse.action == MouseAction::Move ?
        std::optional<Point>{mouse.position} : std::nullopt);
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
    if (!overlays_.modalOpen() &&
        toasts_.handleMouse(mouse,backend_->size(),overlays_.toastOccupiedHeight(backend_->size())))
        return true;
    if (component_ && reconciler_.root())
    {
        if (auto* target = overlays_.handleMouse(mouse,backend_->size()))
        {
            overlays_.dismissExcept(target);
            if (target->type() != Component::Type::ContextMenu &&
                target->type() != Component::Type::Toast)
                focus_.focus(std::to_string(target->id()));
            return true;
        }
        overlays_.dismissOutside(mouse,backend_->size());
        if (mouse.action == MouseAction::Press && mouse.button == MouseButton::Right)
        {
            if (auto* menu = reconciler_.root()->contextTarget(mouse.position))
            {
                overlays_.dismissExcept(menu);
                return menu->openContextMenu(mouse.position,backend_->size());
            }
        }
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
    if (split_drag_id_ != 0)
    {
        if (mouse.action == MouseAction::Release)
        {
            split_drag_id_ = 0;
            updateHover(mouse);
            updateMousePointer(mouse);
            return true;
        }
        if (mouse.action == MouseAction::Move)
        {
            if (component_ && reconciler_.root())
                if (auto* target = reconciler_.root()->findById(split_drag_id_))
                    return target->dragSplit(mouse.position);
            split_drag_id_ = 0;
        }
    }
    if (slider_drag_id_ != 0)
    {
        if (mouse.action == MouseAction::Release)
        {
            slider_drag_id_ = 0;
            return true;
        }
        if (mouse.action == MouseAction::Move)
        {
            if (component_ && reconciler_.root())
                if (auto* target = reconciler_.root()->findById(slider_drag_id_))
                    return target->handleSliderMouse(mouse.position);
            slider_drag_id_ = 0;
        }
    }
    if (mouse.action == MouseAction::Press && mouse.button == MouseButton::Left &&
        component_ && reconciler_.root())
    {
        if (auto* target = reconciler_.root()->splitTarget(mouse.position))
        {
            split_drag_id_ = target->id();
            pressed_button_id_.clear();
            focus_.focus(std::to_string(target->id()));
            return true;
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
        hit->interactiveControl() || hit->sliderInteractive())) hit = nullptr;
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
        if (hit && hit->sliderInteractive())
        {
            slider_drag_id_ = hit->id();
            pressed_button_id_.clear();
            return hit->handleSliderMouse(mouse.position);
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
                    hit->interactiveControl() || hit->sliderInteractive() ||
                    (hit->type() == Component::Type::Tabs && !hit->disabled()))
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
                    hit->interactiveControl() || hit->sliderInteractive() ||
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
    if (event.key != Key::Resize && event.key != Key::Mouse)
    {
        for (const auto& shortcut : shortcuts_)
        {
            if (!shortcut.shortcut.matches(event)) continue;
            const auto callback = shortcut.callback;
            callback();
            return true;
        }
    }
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
    auto next_progress_frame = std::chrono::steady_clock::now();
    while (running_)
    {
        if (overlays_.tick()) invalidate();
        (void)toasts_.tick();
        const auto now = std::chrono::steady_clock::now();
        if (now >= next_progress_frame)
        {
            if (reconciler_.root() && reconciler_.root()->hasIndeterminateProgress()) invalidate();
            next_progress_frame = now+std::chrono::milliseconds(90);
        }
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

