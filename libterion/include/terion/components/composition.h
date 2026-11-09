/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#ifndef TERION_COMPONENTS_COMPOSITION_H
#define TERION_COMPONENTS_COMPOSITION_H

#include <concepts>
#include <functional>
#include <ranges>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <vector>
#include <terion/components/fragment.h>

namespace terion {

// The inactive subtree is absent from reconciliation (including hooks/effects).
[[nodiscard]] inline Component When(bool condition,Component child)
{
    if (condition) return Fragment({.children = {std::move(child)}});
    return Fragment({});
}

// Map a range to keyed child declarations. Unique nonempty keys are mandatory
// because stateful list items must retain their identity across reordering.
template<std::ranges::input_range Range,typename Render>
    requires std::invocable<Render&,std::ranges::range_reference_t<Range>> &&
        std::convertible_to<std::invoke_result_t<Render&,std::ranges::range_reference_t<Range>>,Component>
[[nodiscard]] Component ForEach(Range&& items,Render&& render)
{
    std::vector<Component> children;
    if constexpr (std::ranges::sized_range<Range>) children.reserve(std::ranges::size(items));
    std::unordered_set<std::string> keys;
    for (auto&& item : items)
    {
        auto child = std::invoke(render,item);
        const auto& key = child.props().key;
        if (key.empty() || !keys.insert(key).second)
            throw std::invalid_argument("ForEach() requires unique nonempty child keys");
        children.push_back(std::move(child));
    }
    return Fragment({.children = std::move(children)});
}

} /* End of namespace terion */

#endif /* TERION_COMPONENTS_COMPOSITION_H */
