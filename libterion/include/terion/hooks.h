/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#ifndef TERION_HOOKS_H
#define TERION_HOOKS_H

#include <any>
#include <functional>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>
#include <terion/component.h>
#include <terion/state.h>

namespace terion {

namespace detail {

struct HookContext
{
    std::vector<std::any>& slots;
    std::size_t position = 0;
};

inline thread_local HookContext* current_hook_context = nullptr;

class HookScope
{
public:
    explicit HookScope(HookContext& context) : previous_(current_hook_context)
    {
        current_hook_context = &context;
    }
    ~HookScope() { current_hook_context = previous_; }
    HookScope(const HookScope&) = delete;
    HookScope& operator=(const HookScope&) = delete;

private:
    HookContext* previous_;
};

template<auto Function>
inline constexpr int component_identity = 0;

} /* End of namespace detail */

template<typename T>
class StateSetter
{
public:
    explicit StateSetter(State<T> state) : state_(std::move(state)) {}

    void operator()(T value) const { state_.set(std::move(value)); }

    template<typename F>
        requires std::invocable<F&,T> && std::convertible_to<std::invoke_result_t<F&,T>,T>
    void operator()(F&& update) const { state_.update(std::forward<F>(update)); }

private:
    mutable State<T> state_;
}; /* End of class StateSetter */

template<typename T>
[[nodiscard]] auto useState(T initial)
{
    auto* context = detail::current_hook_context;
    if (!context) throw std::logic_error("useState() requires a mounted function component");
    const auto index = context->position++;
    if (index == context->slots.size()) context->slots.emplace_back(State<T>(std::move(initial)));
    auto* state = std::any_cast<State<T>>(&context->slots.at(index));
    if (!state) throw std::logic_error("useState() hook type/order changed between renders");
    return std::pair<T,StateSetter<T>>{state->get(),StateSetter<T>(*state)};
}

// Function definitions stay ordinary C++. Instantiation is deferred until reconciliation.
template<auto Function,typename... Args>
[[nodiscard]] Component ComponentOf(std::string key,Args&&... args)
{
    auto bound = [arguments = std::make_tuple(std::forward<Args>(args)...)]() mutable -> Component {
        return std::apply(Function,arguments);
    };
    return Component({.type = Component::Type::Function,.key = std::move(key),
        .render_function = std::move(bound),
        .function_identity = &detail::component_identity<Function>});
}

} /* End of namespace terion */

#endif /* TERION_HOOKS_H */
