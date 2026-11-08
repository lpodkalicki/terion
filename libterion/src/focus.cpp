/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <terion/focus.h>
#include <algorithm>
#include <stdexcept>

using namespace terion;

void FocusManager::add(std::string id, EventHandler handler, std::string scope)
{
    if (id.empty() || !handler) throw std::invalid_argument("focus target needs an id and handler");
    const bool needs_focus = current().empty() && scope == active_scope_;
    const auto it = std::find_if(entries_.begin(),entries_.end(),[&](const Entry& entry) { return entry.id == id; });
    if (it != entries_.end())
    {
        it->handler = std::move(handler);
        it->scope = std::move(scope);
        return;
    }
    entries_.push_back({.id = std::move(id),.handler = std::move(handler),.scope = std::move(scope)});
    if (needs_focus) focused_ = entries_.size()-1;
}

bool FocusManager::remove(const std::string_view id)
{
    const auto it = std::find_if(entries_.begin(),entries_.end(),[&](const Entry& entry) { return entry.id == id; });
    if (it == entries_.end()) return false;
    const auto index = static_cast<std::size_t>(std::distance(entries_.begin(),it));
    entries_.erase(it);
    if (entries_.empty()) focused_ = 0;
    else if (index < focused_ || focused_ >= entries_.size()) --focused_;
    return true;
}

bool FocusManager::focus(const std::string_view id)
{
    for (std::size_t i = 0; i < entries_.size(); ++i)
    {
        if (entries_[i].id == id && entries_[i].scope == active_scope_) { focused_ = i; return true; }
    }
    return false;
}

bool FocusManager::next()
{
    if (entries_.empty()) return false;
    for (std::size_t n = 1; n <= entries_.size(); ++n)
    {
        const auto index = (focused_ + n) % entries_.size();
        if (entries_[index].scope == active_scope_) { focused_ = index; return true; }
    }
    return false;
}

bool FocusManager::previous()
{
    if (entries_.empty()) return false;
    for (std::size_t n = 1; n <= entries_.size(); ++n)
    {
        const auto index = (focused_ + entries_.size() - n % entries_.size()) % entries_.size();
        if (entries_[index].scope == active_scope_) { focused_ = index; return true; }
    }
    return false;
}

bool FocusManager::dispatch(const Event& event)
{
    if (event.key == Key::Tab) return next();
    if (entries_.empty() || entries_[focused_].scope != active_scope_) return false;
    return entries_[focused_].handler(event);
}

std::string_view FocusManager::current() const noexcept
{
    return entries_.empty() || entries_[focused_].scope != active_scope_ ?
        std::string_view{} : std::string_view(entries_[focused_].id);
}

bool FocusManager::hasTargetsInActiveScope() const noexcept
{
    return std::any_of(entries_.begin(),entries_.end(),[&](const Entry& entry) {
        return entry.scope == active_scope_;
    });
}

void FocusManager::setActiveScope(std::string scope)
{
    if (active_scope_ == scope) return;
    active_scope_ = std::move(scope);
    if (current().empty())
    {
        for (std::size_t i = 0; i < entries_.size(); ++i)
            if (entries_[i].scope == active_scope_) { focused_ = i; break; }
    }
}
