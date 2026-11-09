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
    virtual void cancelPending() noexcept = 0;

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
        if (!always && committed_dependencies_ && *committed_dependencies_ == dependencies)
        {
            cancelPending();
            return;
        }
        staged_dependencies_ = std::move(dependencies);
        pending_ = std::move(effect);
    }

    void commit() override
    {
        if (!pending_) return;
        auto effect = std::move(pending_);
        pending_ = {};
        cleanup();
        cleanup_ = effect();
        committed_dependencies_ = std::move(staged_dependencies_);
        staged_dependencies_.reset();
    }

    void cancelPending() noexcept override
    {
        pending_ = {};
        staged_dependencies_.reset();
    }

private:
    std::optional<Dependencies> committed_dependencies_;
    std::optional<Dependencies> staged_dependencies_;
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

template<typename Function>
inline constexpr int callable_component_identity = 0;

template<typename Signature>
struct CallableProps;

template<typename Owner,typename Result,typename Arg>
struct CallableProps<Result (Owner::*)(Arg) const>
{
    using Type = std::decay_t<Arg>;
};

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

// A component declaration is lightweight; its callable runs only during reconciliation.
template<typename Function>
class ComponentFactory
{
public:
    explicit ComponentFactory(Function function) : function_(std::move(function)) {}

    // A concrete Props parameter makes C++20 designated initializers deducible.
    template<typename F = Function>
    [[nodiscard]] Component operator()(typename detail::CallableProps<decltype(&F::operator())>::Type props) const
    {
        return defer(std::make_tuple(std::move(props)));
    }

    template<typename... Args>
        requires std::invocable<const Function&,std::decay_t<Args>...> &&
            std::convertible_to<std::invoke_result_t<const Function&,std::decay_t<Args>...>,Component>
    [[nodiscard]] Component operator()(Args&&... args) const
    {
        return defer(std::make_tuple(std::forward<Args>(args)...));
    }

private:
    template<typename Arguments>
    [[nodiscard]] Component defer(Arguments arguments) const
    {
        auto bound = [function = function_,arguments = std::move(arguments)]()
            mutable -> Component { return std::apply(function,arguments); };
        return Component({.type = Component::Type::Function,
            .render_function = std::move(bound),
            .function_identity = &detail::callable_component_identity<Function>});
    }
    Function function_;
};

template<typename Function>
[[nodiscard]] auto component(Function&& function)
{
    return ComponentFactory<std::decay_t<Function>>(std::forward<Function>(function));
}

} /* End of namespace terion */

#endif /* TERION_HOOKS_H */
