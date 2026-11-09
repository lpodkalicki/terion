/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <terion/overlay.h>
#include <algorithm>

using namespace terion;

void OverlayManager::collect(RenderNode* root)
{
    portals_.clear();
    if (!root) return;
    const auto visit = [&](const auto& self, RenderNode& node) -> void {
        if (!node.tabPanelActive()) return;
        const auto add = [&](const OverlayLayer layer, const int z_index) {
            portals_.push_back(Portal{.owner = &node,.layer = layer,.z_index = z_index});
        };
        if (node.type() == Component::Type::Select || node.type() == Component::Type::DropdownMenu ||
            node.type() == Component::Type::ContextMenu || node.type() == Component::Type::Tooltip)
            add(OverlayLayer::Popup,0);
        if (node.type() == Component::Type::Dialog) add(OverlayLayer::Modal,100);
        if (node.type() == Component::Type::Toast) add(OverlayLayer::Notification,200);
        for (const auto& child : node.children()) self(self,*child);
    };
    visit(visit,*root);
    sortPortals();
    if (auto* modal = activeDialog())
        for (const auto& portal : portals_)
            if (portal.owner->selectOpen() && !modal->findById(portal.owner->id()))
                portal.owner->closeSelect();
}

void OverlayManager::addPortal(RenderNode& owner, const int z_index)
{
    const OverlayLayer layer = owner.type() == Component::Type::Dialog ? OverlayLayer::Modal :
        owner.type() == Component::Type::Toast ? OverlayLayer::Notification : OverlayLayer::Popup;
    portals_.push_back(Portal{.owner = &owner,.layer = layer,.z_index = z_index});
    sortPortals();
}

void OverlayManager::sortPortals()
{
    std::stable_sort(portals_.begin(),portals_.end(),[](const Portal& a, const Portal& b) {
        if (a.layer != b.layer) return a.layer < b.layer;
        return a.z_index < b.z_index;
    });
}

bool OverlayManager::belongsToActiveModal(const RenderNode& owner) const noexcept
{
    const auto* modal = activeDialog();
    return !modal || modal->findById(owner.id());
}

void OverlayManager::paint(Painter& painter, const Size screen, const Theme& theme,
    const std::string_view focused_id) const
{
    const auto* modal = activeDialog();
    if (modal)
    {
        modal->paintDialog(painter,screen,modal->resolvedTheme(theme),focused_id);
        for (const auto& portal : portals_)
            if (portal.owner->selectOpen() && belongsToActiveModal(*portal.owner))
                portal.owner->paintOverlay(painter,screen,portal.owner->resolvedTheme(theme));
        return;
    }

    for (const auto& portal : portals_)
        if (portal.owner->selectOpen())
            portal.owner->paintOverlay(painter,screen,portal.owner->resolvedTheme(theme));

    if (hover_position_)
    {
        const Point point = *hover_position_;
        const bool menu_open = std::any_of(portals_.begin(),portals_.end(),
            [](const Portal& portal) { return portal.owner->selectOpen(); });
        if (!menu_open)
            for (auto it = portals_.rbegin(); it != portals_.rend(); ++it)
                if (it->owner->tooltipContains(point))
                {
                    it->owner->paintTooltip(painter,point,screen,it->owner->resolvedTheme(theme));
                    break;
                }
    }
    int offset = 0;
    for (const auto& portal : portals_)
        if (portal.owner->toastOpen())
        {
            const Rect popup = portal.owner->toastRect(screen,offset);
            portal.owner->paintToast(painter,screen,portal.owner->resolvedTheme(theme),offset);
            if (popup.height > 0) offset += popup.height+1;
        }
}

bool OverlayManager::tick(const std::chrono::steady_clock::time_point now)
{
    bool changed = false;
    for (const auto& portal : portals_)
        if (portal.owner->expireToast(now)) changed = true;
    return changed;
}

int OverlayManager::toastOccupiedHeight(const Size screen) const noexcept
{
    int offset = 0;
    for (const auto& portal : portals_)
        if (portal.owner->toastOpen())
        {
            const Rect rect = portal.owner->toastRect(screen,offset);
            if (rect.height <= 0) break;
            offset += rect.height+1;
        }
    return offset;
}

bool OverlayManager::modalOpen() const noexcept
{
    for (const auto& portal : portals_)
        if (portal.owner->dialogOpen()) return true;
    return false;
}

RenderNode* OverlayManager::activeDialog() const noexcept
{
    for (auto it = portals_.rbegin(); it != portals_.rend(); ++it)
        if (it->owner->dialogOpen()) return it->owner;
    return nullptr;
}

RenderNode* OverlayManager::activePopup() const noexcept
{
    for (auto it = portals_.rbegin(); it != portals_.rend(); ++it)
        if (it->owner->selectOpen() && belongsToActiveModal(*it->owner)) return it->owner;
    return nullptr;
}

bool OverlayManager::acceptsInput(const RenderNode& node) const noexcept
{
    return belongsToActiveModal(node);
}

bool OverlayManager::handleKey(const Event& event)
{
    if (auto* popup = activePopup(); popup &&
        (activeDialog() || popup->type() == Component::Type::ContextMenu))
    {
        (void)popup->handleSelectKey(event);
        return true;
    }
    if (auto* modal = activeDialog()) return modal->handleDialogKey(event);
    return false;
}

RenderNode* OverlayManager::handleMouse(const MouseEvent& mouse, const Size screen)
{
    if (auto* modal = activeDialog())
    {
        for (auto it = portals_.rbegin(); it != portals_.rend(); ++it)
            if (it->owner->selectOpen() && belongsToActiveModal(*it->owner) &&
                (it->owner->selectContains(mouse.position,screen) || it->owner->selectDragging()) &&
                it->owner->handleSelectMouse(mouse,screen)) return it->owner;
        if (mouse.action == MouseAction::Press &&
            (mouse.button == MouseButton::Left || mouse.button == MouseButton::Right))
            for (const auto& portal : portals_)
                if (portal.owner->selectOpen() && belongsToActiveModal(*portal.owner))
                    portal.owner->closeSelect();
        (void)modal->handleDialogMouse(mouse,screen);
        return modal;
    }
    if (mouse.action == MouseAction::Press && mouse.button == MouseButton::Left)
    {
        int offset = 0;
        for (const auto& portal : portals_)
            if (portal.owner->toastOpen())
            {
                const Rect popup = portal.owner->toastRect(screen,offset);
                if (mouse.position.x >= popup.x && mouse.position.x < popup.x+popup.width &&
                    mouse.position.y >= popup.y && mouse.position.y < popup.y+popup.height)
                {
                    portal.owner->closeToast();
                    return portal.owner;
                }
                if (popup.height > 0) offset += popup.height+1;
            }
    }
    for (auto it = portals_.rbegin(); it != portals_.rend(); ++it)
    {
        auto* portal = it->owner;
        if (portal->selectOpen() && (portal->selectContains(mouse.position,screen) || portal->selectDragging()) &&
            portal->handleSelectMouse(mouse,screen)) return portal;
    }
    return nullptr;
}

void OverlayManager::dismissOutside(const MouseEvent& mouse, const Size screen)
{
    if (modalOpen()) return;
    if (mouse.action != MouseAction::Press ||
        (mouse.button != MouseButton::Left && mouse.button != MouseButton::Right)) return;
    for (const auto& entry : portals_)
    {
        auto* portal = entry.owner;
        if (portal->selectOpen() && !portal->selectContains(mouse.position,screen) &&
            (portal->type() == Component::Type::ContextMenu || !portal->hitTest(mouse.position)))
            portal->closeSelect();
    }
}

void OverlayManager::dismissExcept(const RenderNode* active)
{
    for (const auto& portal : portals_)
        if (portal.owner != active) portal.owner->closeSelect();
}
