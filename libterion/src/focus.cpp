/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <terion/focus.h>
#include <algorithm>
#include <stdexcept>

using namespace terion;

void FocusManager::add(std::string id, EventHandler handler)
{
    if (id.empty() || !handler) throw std::invalid_argument("focus target needs an id and handler");
    const auto it = std::find_if(entries_.begin(),entries_.end(),[&](const Entry& entry) { return entry.id == id; });
    if (it != entries_.end())
    {
        it->handler = std::move(handler);
        return;
    }
    entries_.push_back({.id = std::move(id),.handler = std::move(handler)});
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
        if (entries_[i].id == id) { focused_ = i; return true; }
    }
    return false;
}

bool FocusManager::next()
{
    if (entries_.empty()) return false;
    focused_ = (focused_ + 1) % entries_.size();
    return true;
}

bool FocusManager::previous()
{
    if (entries_.empty()) return false;
    focused_ = (focused_ + entries_.size() - 1) % entries_.size();
    return true;
}

bool FocusManager::dispatch(const Event& event)
{
    if (event.key == Key::Tab) return next();
    if (entries_.empty()) return false;
    return entries_[focused_].handler(event);
}

std::string_view FocusManager::current() const noexcept
{
    return entries_.empty() ? std::string_view{} : std::string_view(entries_[focused_].id);
}
