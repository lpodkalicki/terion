/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <terion/state.h>

using namespace terion;

thread_local DependencyTracker* DependencyTracker::current_ = nullptr;

std::size_t Signal::subscribe(Callback callback)
{
    std::lock_guard lock(mutex_);
    const auto id = next_id_++;
    subscribers_.emplace(id,std::move(callback));
    return id;
}

void Signal::unsubscribe(const std::size_t id)
{
    std::lock_guard lock(mutex_);
    subscribers_.erase(id);
}

void Signal::notify()
{
    std::vector<Callback> callbacks;
    {
        std::lock_guard lock(mutex_);
        for (const auto& [id,callback] : subscribers_) callbacks.push_back(callback);
    }
    for (const auto& callback : callbacks) callback();
}

DependencyTracker::~DependencyTracker()
{
    clear();
}

void DependencyTracker::clear()
{
    for (const auto& [signal,id] : subscriptions_) signal->unsubscribe(id);
    subscriptions_.clear();
    observed_.clear();
}

DependencyTracker::Scope::Scope(DependencyTracker& tracker) : previous_(current_)
{
    tracker.clear();
    current_ = &tracker;
}

DependencyTracker::Scope::~Scope()
{
    current_ = previous_;
}

void DependencyTracker::track(const std::shared_ptr<Signal>& signal)
{
    if (current_) current_->add(signal);
}

void DependencyTracker::add(const std::shared_ptr<Signal>& signal)
{
    if (!observed_.insert(signal.get()).second) return;
    subscriptions_.emplace_back(signal,signal->subscribe(callback_));
}
