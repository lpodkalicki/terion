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
    void update(const Component& component);
    void clear() noexcept { root_.reset(); }
    [[nodiscard]] const RenderNode* root() const noexcept { return root_.get(); }

private:
    void reconcile(std::unique_ptr<RenderNode>& node, const Component& component);

    std::unique_ptr<RenderNode> root_;
    std::uint64_t next_id_ = 1;
}; /* End of class Reconciler */

} /* End of namespace terion */

#endif /* TERION_RECONCILER_H */
