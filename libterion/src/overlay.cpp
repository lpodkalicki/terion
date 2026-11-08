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
        if (node.type() == Component::Type::Select) addPortal(node);
        if (node.type() == Component::Type::Dialog) addPortal(node,100);
        for (const auto& child : node.children()) self(self,*child);
    };
    visit(visit,*root);
}

void OverlayManager::addPortal(RenderNode& owner, const int z_index)
{
    portals_.push_back(Portal{.owner = &owner,.zIndex = z_index});
    std::stable_sort(portals_.begin(),portals_.end(),[](const Portal& a, const Portal& b) {
        return a.zIndex < b.zIndex;
    });
}

void OverlayManager::paint(Painter& painter, const Size screen, const Theme& theme,
    const std::string_view focused_id) const
{
    const auto* modal = activeDialog();
    for (const auto& portal : portals_)
    {
        if (portal.owner->selectOpen() && (!modal || !modal->findById(portal.owner->id())))
            portal.owner->paintOverlay(painter,screen,theme);
        if (portal.owner->dialogOpen()) portal.owner->paintDialog(painter,screen,theme,focused_id);
    }
    if (modal)
        for (const auto& portal : portals_)
            if (portal.owner->selectOpen() && modal->findById(portal.owner->id()))
                portal.owner->paintOverlay(painter,screen,theme);
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

bool OverlayManager::handleKey(const Event& event)
{
    for (auto it = portals_.rbegin(); it != portals_.rend(); ++it)
        if (it->owner->dialogOpen()) return it->owner->handleDialogKey(event);
    return false;
}

RenderNode* OverlayManager::handleMouse(const MouseEvent& mouse, const Size screen)
{
    if (const auto* modal = activeDialog())
        for (auto it = portals_.rbegin(); it != portals_.rend(); ++it)
            if (it->owner->selectOpen() && modal->findById(it->owner->id()) &&
                (it->owner->selectContains(mouse.position,screen) || it->owner->selectDragging()) &&
                it->owner->handleSelectMouse(mouse,screen)) return it->owner;
    for (auto it = portals_.rbegin(); it != portals_.rend(); ++it)
    {
        auto* portal = it->owner;
        if (portal->dialogOpen())
        {
            portal->handleDialogMouse(mouse,screen);
            return portal;
        }
        if (portal->selectOpen() && (portal->selectContains(mouse.position,screen) || portal->selectDragging()) &&
            portal->handleSelectMouse(mouse,screen)) return portal;
    }
    return nullptr;
}

void OverlayManager::dismissOutside(const MouseEvent& mouse, const Size screen)
{
    if (mouse.action != MouseAction::Press || mouse.button != MouseButton::Left) return;
    for (const auto& entry : portals_)
    {
        auto* portal = entry.owner;
        if (portal->selectOpen() && !portal->selectContains(mouse.position,screen) &&
            !portal->hitTest(mouse.position)) portal->closeSelect();
    }
}

void OverlayManager::dismissExcept(const RenderNode* active)
{
    for (const auto& portal : portals_)
        if (portal.owner != active) portal.owner->closeSelect();
}
