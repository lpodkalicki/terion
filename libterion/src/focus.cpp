/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <terion/focus.h>
#include <algorithm>
#include <stdexcept>
#include <unordered_map>

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
    const bool blurred = focused_ >= entries_.size();
    entries_.erase(it);
    if (blurred) focused_ = entries_.size();
    else if (entries_.empty()) focused_ = 0;
    else if (index < focused_) --focused_;
    else if (focused_ >= entries_.size()) --focused_;
    return true;
}

void FocusManager::reorder(const std::vector<std::string>& ids)
{
    const std::string selected(current());
    std::unordered_map<std::string,std::size_t> ranks;
    ranks.reserve(ids.size());
    for (std::size_t i = 0; i < ids.size(); ++i) ranks.emplace(ids[i],i);
    std::vector<std::size_t> positions;
    std::vector<Entry> managed;
    for (std::size_t i = 0; i < entries_.size(); ++i)
        if (ranks.contains(entries_[i].id))
        {
            positions.push_back(i);
            managed.push_back(std::move(entries_[i]));
        }
    std::stable_sort(managed.begin(),managed.end(),[&](const Entry& a, const Entry& b) {
        return ranks.at(a.id) < ranks.at(b.id);
    });
    for (std::size_t i = 0; i < positions.size(); ++i)
        entries_[positions[i]] = std::move(managed[i]);
    if (!selected.empty()) (void)focus(selected);
}

bool FocusManager::focus(const std::string_view id)
{
    for (std::size_t i = 0; i < entries_.size(); ++i)
    {
        if (entries_[i].id == id && entries_[i].scope == active_scope_) { focused_ = i; return true; }
    }
    return false;
}

bool FocusManager::focusFirstInScope()
{
    for (std::size_t i = 0; i < entries_.size(); ++i)
        if (entries_[i].scope == active_scope_)
        {
            focused_ = i;
            return true;
        }
    focused_ = entries_.size();
    return false;
}

bool FocusManager::next()
{
    if (entries_.empty()) return false;
    for (std::size_t n = 1; n <= entries_.size(); ++n)
    {
        const auto index = focused_ >= entries_.size() ? n-1 : (focused_ + n) % entries_.size();
        if (entries_[index].scope == active_scope_) { focused_ = index; return true; }
    }
    return false;
}

bool FocusManager::previous()
{
    if (entries_.empty()) return false;
    for (std::size_t n = 1; n <= entries_.size(); ++n)
    {
        const auto index = focused_ >= entries_.size() ? entries_.size()-n :
            (focused_ + entries_.size() - n % entries_.size()) % entries_.size();
        if (entries_[index].scope == active_scope_) { focused_ = index; return true; }
    }
    return false;
}

bool FocusManager::dispatch(const Event& event)
{
    if (!event.ctrl && !event.alt && (event.key == Key::Tab || event.key == Key::BackTab))
        return event.key == Key::BackTab || event.shift ? previous() : next();
    if (focused_ >= entries_.size() || entries_[focused_].scope != active_scope_) return false;
    return entries_[focused_].handler(event);
}

std::string_view FocusManager::current() const noexcept
{
    return focused_ >= entries_.size() || entries_[focused_].scope != active_scope_ ?
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
