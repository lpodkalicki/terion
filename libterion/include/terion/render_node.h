/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#ifndef TERION_RENDER_NODE_H
#define TERION_RENDER_NODE_H

#include <cstdint>
#include <memory>
#include <string>
#include <functional>
#include <string_view>
#include <vector>
#include <terion/component.h>
#include <terion/layout.h>
#include <terion/render/painter.h>

namespace terion {

class RenderNode
{
public:
    [[nodiscard]] std::uint64_t id() const noexcept { return id_; }
    [[nodiscard]] Component::Type type() const noexcept { return type_; }
    [[nodiscard]] const std::string& key() const noexcept { return key_; }
    [[nodiscard]] const std::string& text() const noexcept { return text_; }
    [[nodiscard]] const std::vector<std::unique_ptr<RenderNode>>& children() const noexcept { return children_; }
    [[nodiscard]] Rect bounds() const noexcept { return bounds_; }
    [[nodiscard]] const RenderNode* hitTest(Point position) const noexcept;
    [[nodiscard]] bool disabled() const noexcept { return disabled_; }
    [[nodiscard]] const std::function<void()>& onClick() const noexcept { return on_click_; }

    [[nodiscard]] Size measure(const Constraints& constraints) const;
    void layout(const Constraints& constraints, Point origin = {});
    void paint(Painter& painter, std::string_view focused_id = {}, std::string_view hovered_id = {}) const;
    [[nodiscard]] int height() const noexcept;

private:
    friend class Reconciler;

    RenderNode(std::uint64_t id, Component::Type type, std::string key) :
        id_(id), type_(type), key_(std::move(key))
    {}

    std::uint64_t id_;
    Component::Type type_;
    std::string key_;
    std::string text_;
    LayoutStyle style_;
    TextStyle text_style_;
    std::function<void()> on_click_;
    bool disabled_ = false;
    Rect bounds_{};
    std::vector<std::unique_ptr<RenderNode>> children_;
}; /* End of class RenderNode */

} /* End of namespace terion */

#endif /* TERION_RENDER_NODE_H */
