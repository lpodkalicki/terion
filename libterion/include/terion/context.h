/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#ifndef TERION_CONTEXT_H
#define TERION_CONTEXT_H

#include <any>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <terion/component.h>
#include <terion/components/fragment.h>
#include <terion/hooks.h>

namespace terion {

namespace detail {

inline thread_local std::unordered_map<const void*,std::vector<std::any>> context_values;

} /* End of namespace detail */

template<typename T>
class Context
{
public:
    explicit Context(T fallback) : identity_(std::make_shared<int>(0)), fallback_(std::move(fallback)) {}

    [[nodiscard]] const void* identity() const noexcept { return identity_.get(); }
    [[nodiscard]] const T& fallback() const noexcept { return fallback_; }
    [[nodiscard]] std::shared_ptr<int> token() const noexcept { return identity_; }

private:
    std::shared_ptr<int> identity_;
    T fallback_;
}; /* End of class Context */

template<typename T>
[[nodiscard]] T useContext(const Context<T>& context)
{
    if (!detail::current_hook_context)
        throw std::logic_error("useContext() requires a mounted function component");
    const auto found = detail::context_values.find(context.identity());
    if (found == detail::context_values.end() || found->second.empty()) return context.fallback();
    return std::any_cast<T>(found->second.back());
}

template<typename T>
[[nodiscard]] Component ContextProvider(const Context<T>& context,T value,Component child,
    std::string key = {})
{
    auto token = context.token();
    return Component({.type = Component::Type::ContextProvider,.key = std::move(key),
        .children = {std::move(child)},
        .enter_context = [token,value = std::move(value)] {
            detail::context_values[token.get()].emplace_back(value);
        },
        .exit_context = [token] {
            const auto it = detail::context_values.find(token.get());
            if (it == detail::context_values.end() || it->second.empty()) return;
            it->second.pop_back();
            if (it->second.empty()) detail::context_values.erase(it);
        }});
}

// Multiple direct children share one context scope without a layout wrapper.
template<typename T>
[[nodiscard]] Component ContextProvider(const Context<T>& context,T value,
    std::vector<Component> children,std::string key = {})
{
    return ContextProvider(context,std::move(value),
        Fragment({.children = std::move(children)}),std::move(key));
}

} /* End of namespace terion */

#endif /* TERION_CONTEXT_H */
