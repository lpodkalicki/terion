/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#ifndef TERION_RECONCILER_H
#define TERION_RECONCILER_H

#include <memory>
#include <terion/component.h>
#include <terion/render_node.h>

namespace terion {

class Reconciler
{
public:
    struct Stats
    {
        std::size_t visited = 0;
        std::size_t created = 0;
        std::size_t reused = 0;
        std::size_t removed = 0;
        std::size_t layout_dirty = 0;
        std::size_t paint_dirty = 0;
    }; /* End of struct Stats */

    void update(const Component& component);
    void clear() noexcept { root_.reset(); }
    [[nodiscard]] RenderNode* root() noexcept { return root_.get(); }
    [[nodiscard]] const RenderNode* root() const noexcept { return root_.get(); }
    [[nodiscard]] const Stats& stats() const noexcept { return stats_; }

private:
    void reconcile(std::unique_ptr<RenderNode>& node, const Component& component);

    std::unique_ptr<RenderNode> root_;
    std::uint64_t next_id_ = 1;
    Stats stats_{};
}; /* End of class Reconciler */

} /* End of namespace terion */

#endif /* TERION_RECONCILER_H */
