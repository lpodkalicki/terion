/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#ifndef TERION_OVERLAY_H
#define TERION_OVERLAY_H

#include <vector>
#include <optional>
#include <terion/render_node.h>

namespace terion {

enum class OverlayLayer { Popup, Modal, Notification };

struct Portal
{
    RenderNode* owner = nullptr;
    OverlayLayer layer = OverlayLayer::Popup;
    int z_index = 0;
}; /* End of struct Portal */

// A portal renders above the retained tree without changing its layout.
// Entries are non-owning and must be rebuilt after each reconciliation.
class OverlayManager
{
public:
    void collect(RenderNode* root);
    void addPortal(RenderNode& owner, int z_index = 0);
    void paint(Painter& painter, Size screen, const Theme& theme, std::string_view focused_id = {}) const;
    [[nodiscard]] RenderNode* handleMouse(const MouseEvent& mouse, Size screen);
    bool handleKey(const Event& event);
    [[nodiscard]] bool modalOpen() const noexcept;
    [[nodiscard]] RenderNode* activeDialog() const noexcept;
    [[nodiscard]] RenderNode* activePopup() const noexcept;
    [[nodiscard]] bool acceptsInput(const RenderNode& node) const noexcept;
    void dismissOutside(const MouseEvent& mouse, Size screen);
    void dismissExcept(const RenderNode* active);
    void setHover(std::optional<Point> position) noexcept { hover_position_ = position; }
    bool tick(std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now());
    [[nodiscard]] int toastOccupiedHeight(Size screen) const noexcept;

private:
    void sortPortals();
    [[nodiscard]] bool belongsToActiveModal(const RenderNode& owner) const noexcept;
    std::vector<Portal> portals_;
    std::optional<Point> hover_position_;
}; /* End of class OverlayManager */

} /* End of namespace terion */

#endif /* TERION_OVERLAY_H */
