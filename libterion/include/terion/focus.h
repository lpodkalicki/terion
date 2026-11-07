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

    void add(std::string id, EventHandler handler);
    bool remove(std::string_view id);
    bool focus(std::string_view id);
    bool next();
    bool previous();
    bool dispatch(const Event& event);

    [[nodiscard]] std::string_view current() const noexcept;
    [[nodiscard]] bool empty() const noexcept { return entries_.empty(); }

private:
    struct Entry
    {
        std::string id;
        EventHandler handler;
    }; /* End of struct Entry */

    std::vector<Entry> entries_;
    std::size_t focused_ = 0;
}; /* End of class FocusManager */

} /* End of namespace terion */

#endif /* TERION_FOCUS_H */
