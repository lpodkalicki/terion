/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#ifndef TERION_EVENT_H
#define TERION_EVENT_H

#include <cstdint>
#include <terion/terminal.h>

namespace terion {

enum class EventPhase
{
    Capture,
    Target,
    Bubble
}; /* End of enum EventPhase */

struct ComponentEvent
{
    ComponentEvent(const Event& source, std::uint64_t target_id) : event(source), target(target_id) {}

    Event event;
    std::uint64_t target = 0;
    std::uint64_t current_target = 0;
    EventPhase phase = EventPhase::Target;

    void stopPropagation() noexcept { stopped_ = true; }
    void preventDefault() noexcept { prevented_ = true; }
    [[nodiscard]] bool propagation_stopped() const noexcept { return stopped_; }
    [[nodiscard]] bool default_prevented() const noexcept { return prevented_; }

private:
    bool stopped_ = false;
    bool prevented_ = false;
}; /* End of struct ComponentEvent */

} /* End of namespace terion */

#endif /* TERION_EVENT_H */
