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

void OverlayManager::paint(Painter& painter, const Size screen, const Theme& theme) const
{
    for (const auto& portal : portals_)
        if (portal.owner->selectOpen()) portal.owner->paintOverlay(painter,screen,theme);
}

RenderNode* OverlayManager::handleMouse(const MouseEvent& mouse, const Size screen)
{
    for (auto it = portals_.rbegin(); it != portals_.rend(); ++it)
    {
        auto* portal = it->owner;
        if (portal->selectOpen() && portal->selectContains(mouse.position,screen) &&
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
