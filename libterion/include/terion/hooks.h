/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#ifndef TERION_HOOKS_H
#define TERION_HOOKS_H

#include <any>
#include <concepts>
#include <functional>
#include <memory>
#include <optional>
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

class EffectBase
{
public:
    virtual ~EffectBase() { cleanup(); }
    virtual void commit() = 0;

protected:
    void cleanup() noexcept
    {
        if (!cleanup_) return;
        auto previous = std::move(cleanup_);
        try { previous(); }
        catch (...) { /* Destructors and unmount must not throw. */ }
    }
    std::function<void()> cleanup_;
};

template<typename Dependencies>
class EffectSlot final : public EffectBase
{
public:
    void schedule(std::function<std::function<void()>()> effect,Dependencies dependencies,bool always)
    {
        if (!always && dependencies_ && *dependencies_ == dependencies) return;
        dependencies_ = std::move(dependencies);
        pending_ = std::move(effect);
    }

    void commit() override
    {
        if (!pending_) return;
        auto effect = std::move(pending_);
        pending_ = {};
        cleanup();
        cleanup_ = effect();
    }

private:
    std::optional<Dependencies> dependencies_;
    std::function<std::function<void()>()> pending_;
};

template<typename T,typename Dependencies>
struct MemoSlot
{
    std::optional<T> value;
    Dependencies dependencies;
};

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

template<typename T>
[[nodiscard]] std::shared_ptr<T> useRef(T initial)
{
    auto* context = detail::current_hook_context;
    if (!context) throw std::logic_error("useRef() requires a mounted function component");
    const auto index = context->position++;
    if (index == context->slots.size())
        context->slots.emplace_back(std::make_shared<T>(std::move(initial)));
    auto* ref = std::any_cast<std::shared_ptr<T>>(&context->slots.at(index));
    if (!ref) throw std::logic_error("useRef() hook type/order changed between renders");
    return *ref;
}

template<typename Factory,typename... Dependencies>
    requires (std::equality_comparable<std::decay_t<Dependencies>> && ...)
[[nodiscard]] auto useMemo(Factory&& factory,Dependencies&&... dependencies)
{
    using Value = std::decay_t<std::invoke_result_t<Factory&>>;
    using Deps = std::tuple<std::decay_t<Dependencies>...>;
    using Slot = detail::MemoSlot<Value,Deps>;
    auto* context = detail::current_hook_context;
    if (!context) throw std::logic_error("useMemo() requires a mounted function component");
    const auto index = context->position++;
    Deps next(std::forward<Dependencies>(dependencies)...);
    const bool first = index == context->slots.size();
    if (first)
        context->slots.emplace_back(Slot{std::invoke(factory),std::move(next)});
    auto* slot = std::any_cast<Slot>(&context->slots.at(index));
    if (!slot) throw std::logic_error("useMemo() hook type/order changed between renders");
    if (!first && slot->dependencies != next)
    {
        slot->value.emplace(std::invoke(factory));
        slot->dependencies = std::move(next);
    }
    return *slot->value;
}

template<typename Callback,typename... Dependencies>
[[nodiscard]] auto useCallback(Callback callback,Dependencies&&... dependencies)
{
    return useMemo([&callback] { return callback; },std::forward<Dependencies>(dependencies)...);
}

template<typename Effect,typename... Dependencies>
    requires (std::equality_comparable<std::decay_t<Dependencies>> && ...)
void useEffect(Effect&& effect,Dependencies&&... dependencies)
{
    using Deps = std::tuple<std::decay_t<Dependencies>...>;
    auto* context = detail::current_hook_context;
    if (!context) throw std::logic_error("useEffect() requires a mounted function component");
    const auto index = context->position++;
    if (index == context->slots.size())
        context->slots.emplace_back(std::shared_ptr<detail::EffectBase>(
            std::make_shared<detail::EffectSlot<Deps>>()));
    auto* base = std::any_cast<std::shared_ptr<detail::EffectBase>>(&context->slots.at(index));
    auto slot = base ? std::dynamic_pointer_cast<detail::EffectSlot<Deps>>(*base) : nullptr;
    if (!slot) throw std::logic_error("useEffect() hook type/order changed between renders");
    auto invoke = [callback = std::forward<Effect>(effect)]() mutable -> std::function<void()> {
        if constexpr (std::is_void_v<std::invoke_result_t<Effect&>>)
        {
            std::invoke(callback);
            return {};
        }
        else return std::function<void()>(std::invoke(callback));
    };
    slot->schedule(std::move(invoke),Deps(std::forward<Dependencies>(dependencies)...),
        sizeof...(Dependencies) == 0);
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
