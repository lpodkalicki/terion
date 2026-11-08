/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#ifndef TERION_FOCUS_H
#define TERION_FOCUS_H

#include <functional>
#include <string>
#include <string_view>
#include <vector>
#include <terion/terminal.h>

namespace terion {

class FocusManager
{
public:
    using EventHandler = std::function<bool(const Event&)>;

    void add(std::string id, EventHandler handler, std::string scope = {});
    bool remove(std::string_view id);
    bool focus(std::string_view id);
    void blur() noexcept { focused_ = entries_.size(); }
    bool next();
    bool previous();
    bool dispatch(const Event& event);
    void setActiveScope(std::string scope);
    [[nodiscard]] std::string_view activeScope() const noexcept { return active_scope_; }
    [[nodiscard]] bool hasTargetsInActiveScope() const noexcept;

    [[nodiscard]] std::string_view current() const noexcept;
    [[nodiscard]] bool empty() const noexcept { return entries_.empty(); }

private:
    struct Entry
    {
        std::string id;
        EventHandler handler;
        std::string scope;
    }; /* End of struct Entry */

    std::vector<Entry> entries_;
    std::size_t focused_ = 0;
    std::string active_scope_;
}; /* End of class FocusManager */

} /* End of namespace terion */

#endif /* TERION_FOCUS_H */
