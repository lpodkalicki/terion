/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#ifndef TERION_STATE_H
#define TERION_STATE_H

#include <atomic>
#include <concepts>
#include <functional>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace terion {

class Signal
{
public:
    using Callback = std::function<void()>;

    std::size_t subscribe(Callback callback);
    void unsubscribe(std::size_t id);
    void notify();

private:
    std::mutex mutex_;
    std::unordered_map<std::size_t,Callback> subscribers_;
    std::size_t next_id_ = 1;
}; /* End of class Signal */

class DependencyTracker
{
public:
    explicit DependencyTracker(Signal::Callback callback) : callback_(std::move(callback)) {}
    ~DependencyTracker();

    DependencyTracker(const DependencyTracker&) = delete;
    DependencyTracker& operator=(const DependencyTracker&) = delete;

    class Scope
    {
    public:
        explicit Scope(DependencyTracker& tracker);
        ~Scope();

        Scope(const Scope&) = delete;
        Scope& operator=(const Scope&) = delete;

    private:
        DependencyTracker* previous_;
    }; /* End of class Scope */

    static void track(const std::shared_ptr<Signal>& signal);

private:
    void clear();
    void add(const std::shared_ptr<Signal>& signal);

    Signal::Callback callback_;
    std::vector<std::pair<std::shared_ptr<Signal>,std::size_t>> subscriptions_;
    std::unordered_set<Signal*> observed_;
    static thread_local DependencyTracker* current_;
}; /* End of class DependencyTracker */

template<typename T>
class State
{
public:
    explicit State(T initial) : data_(std::make_shared<Data>(std::move(initial))) {}

    [[nodiscard]] T get() const
    {
        DependencyTracker::track(data_->signal);
        std::lock_guard lock(data_->mutex);
        return data_->value;
    }

    void set(T value)
    {
        {
            std::lock_guard lock(data_->mutex);
            if constexpr (std::equality_comparable<T>)
                if (data_->value == value) return;
            data_->value = std::move(value);
        }
        data_->signal->notify();
    }

    template<typename F>
    void update(F&& function)
    {
        {
            std::lock_guard lock(data_->mutex);
            T value = std::invoke(std::forward<F>(function),data_->value);
            if constexpr (std::equality_comparable<T>)
                if (data_->value == value) return;
            data_->value = std::move(value);
        }
        data_->signal->notify();
    }

private:
    struct Data
    {
        explicit Data(T value) : value(std::move(value)), signal(std::make_shared<Signal>()) {}
        std::mutex mutex;
        T value;
        std::shared_ptr<Signal> signal;
    }; /* End of struct Data */

    std::shared_ptr<Data> data_;
}; /* End of class State */

} /* End of namespace terion */

#endif /* TERION_STATE_H */
