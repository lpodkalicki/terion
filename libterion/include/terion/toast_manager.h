/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#ifndef TERION_TOAST_MANAGER_H
#define TERION_TOAST_MANAGER_H

#include <chrono>
#include <cstdint>
#include <deque>
#include <functional>
#include <string>
#include <terion/components/toast.h>
#include <terion/render/painter.h>
#include <terion/terminal.h>
#include <terion/theme.h>

namespace terion {

struct ToastNotification
{
    std::string message;
    ToastSeverity severity = ToastSeverity::Info;
    int durationMs = 3500; // 0 = persistent until dismissed.
}; /* End of struct ToastNotification */

class ToastManager
{
public:
    explicit ToastManager(std::function<void()> on_change = {}) :
        on_change_(std::move(on_change))
    {}

    [[nodiscard]] std::uint64_t enqueue(ToastNotification notification);
    bool dismiss(std::uint64_t id);
    void clear();
    bool tick(std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now());
    bool handleMouse(const MouseEvent& mouse, Size screen, int bottom_offset = 0);
    void paint(Painter& painter, Size screen, const Theme& theme, int bottom_offset = 0) const;
    [[nodiscard]] std::size_t visibleCount() const noexcept { return visible_.size(); }
    [[nodiscard]] std::size_t pendingCount() const noexcept { return pending_.size(); }
    [[nodiscard]] int occupiedHeight(Size screen) const noexcept;

private:
    struct Item
    {
        std::uint64_t id;
        ToastNotification notification;
        std::chrono::steady_clock::time_point started{};
    }; /* End of struct Item */

    static constexpr std::size_t max_visible_ = 3;
    static constexpr std::size_t max_pending_ = 100;
    std::deque<Item> visible_;
    std::deque<Item> pending_;
    std::uint64_t next_id_ = 1;
    std::function<void()> on_change_;

    void promote(std::chrono::steady_clock::time_point now);
    [[nodiscard]] static Rect itemRect(const Item& item, Size screen, int bottom_offset) noexcept;
}; /* End of class ToastManager */

} /* End of namespace terion */

#endif /* TERION_TOAST_MANAGER_H */
