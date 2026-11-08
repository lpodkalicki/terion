/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#ifndef TERION_RECONCILER_H
#define TERION_RECONCILER_H

#include <memory>
#include <any>
#include <string>
#include <unordered_map>
#include <vector>
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
    void commitEffects();
    void clear() noexcept { root_.reset(); hooks_.clear(); retired_hooks_.clear(); }
    [[nodiscard]] RenderNode* root() noexcept { return root_.get(); }
    [[nodiscard]] const RenderNode* root() const noexcept { return root_.get(); }
    [[nodiscard]] const Stats& stats() const noexcept { return stats_; }

private:
    struct HookRecord
    {
        const void* identity = nullptr;
        std::vector<std::any> slots;
        std::size_t generation = 0;
    };

    [[nodiscard]] Component expand(const Component& component, const std::string& path);
    void reconcile(std::unique_ptr<RenderNode>& node, const Component& component);

    std::unique_ptr<RenderNode> root_;
    std::uint64_t next_id_ = 1;
    Stats stats_{};
    std::unordered_map<std::string,HookRecord> hooks_;
    std::vector<HookRecord> retired_hooks_;
    std::size_t generation_ = 0;
}; /* End of class Reconciler */

} /* End of namespace terion */

#endif /* TERION_RECONCILER_H */
