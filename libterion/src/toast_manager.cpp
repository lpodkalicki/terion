/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#include <terion/toast_manager.h>
#include <terion/utf8.h>
#include <algorithm>
#include <utility>

using namespace terion;

namespace {

int textWidth(const std::string_view value)
{
    int width = 0;
    for (std::size_t i = 0; i < value.size();)
    {
        const auto decoded = decodeUtf8(value,i);
        if (!decoded.bytes) break;
        width += codePointWidth(decoded.code_point);
        i += decoded.bytes;
    }
    return width;
}

std::string clipText(const std::string_view value, const int limit)
{
    std::string result;
    int width = 0;
    for (std::size_t i = 0; i < value.size();)
    {
        const auto decoded = decodeUtf8(value,i);
        if (!decoded.bytes || width+codePointWidth(decoded.code_point) > limit) break;
        result.append(value.substr(i,decoded.bytes));
        width += codePointWidth(decoded.code_point);
        i += decoded.bytes;
    }
    return result;
}

} /* End of anonymous namespace */

std::uint64_t ToastManager::enqueue(ToastNotification notification)
{
    notification.durationMs = std::max(0,notification.durationMs);
    const std::uint64_t id = next_id_++;
    if (pending_.size() >= max_pending_) pending_.pop_front();
    pending_.push_back({.id = id,.notification = std::move(notification)});
    promote(std::chrono::steady_clock::now());
    if (on_change_) on_change_();
    return id;
}

void ToastManager::promote(const std::chrono::steady_clock::time_point now)
{
    while (visible_.size() < max_visible_ && !pending_.empty())
    {
        auto item = std::move(pending_.front());
        pending_.pop_front();
        item.started = now;
        visible_.push_back(std::move(item));
    }
}

bool ToastManager::dismiss(const std::uint64_t id)
{
    const auto before = visible_.size()+pending_.size();
    std::erase_if(visible_,[id](const Item& item) { return item.id == id; });
    std::erase_if(pending_,[id](const Item& item) { return item.id == id; });
    if (before == visible_.size()+pending_.size()) return false;
    promote(std::chrono::steady_clock::now());
    if (on_change_) on_change_();
    return true;
}

void ToastManager::clear()
{
    if (visible_.empty() && pending_.empty()) return;
    visible_.clear();
    pending_.clear();
    if (on_change_) on_change_();
}

bool ToastManager::tick(const std::chrono::steady_clock::time_point now)
{
    const auto before = visible_.size();
    std::erase_if(visible_,[now](const Item& item) {
        return item.notification.durationMs > 0 &&
            now-item.started >= std::chrono::milliseconds(item.notification.durationMs);
    });
    if (visible_.size() == before) return false;
    promote(now);
    if (on_change_) on_change_();
    return true;
}

Rect ToastManager::itemRect(const Item& item, const Size screen, const int bottom_offset) noexcept
{
    if (screen.width <= 0 || screen.height <= bottom_offset) return {};
    const int height = screen.height-bottom_offset >= 3 ? 3 : 1;
    const int width = std::min(screen.width,std::max(12,textWidth(item.notification.message)+7));
    return {.x = std::max(0,screen.width-width-1),
        .y = screen.height-bottom_offset-height,.width = width,.height = height};
}

int ToastManager::occupiedHeight(const Size screen) const noexcept
{
    int offset = 0;
    for (const auto& item : visible_)
    {
        const Rect rect = itemRect(item,screen,offset);
        if (rect.height == 0) break;
        offset += rect.height+1;
    }
    return offset;
}

bool ToastManager::handleMouse(const MouseEvent& mouse, const Size screen, const int bottom_offset)
{
    if (mouse.action != MouseAction::Press || mouse.button != MouseButton::Left) return false;
    int offset = bottom_offset;
    for (const auto& item : visible_)
    {
        const Rect popup = itemRect(item,screen,offset);
        if (mouse.position.x >= popup.x && mouse.position.x < popup.x+popup.width &&
            mouse.position.y >= popup.y && mouse.position.y < popup.y+popup.height)
        {
            const auto id = item.id;
            (void)dismiss(id);
            return true;
        }
        if (popup.height > 0) offset += popup.height+1;
    }
    return false;
}

void ToastManager::paint(Painter& painter, const Size screen, const Theme& theme,
    const int bottom_offset) const
{
    int offset = bottom_offset;
    const auto& palette = theme.palette;
    for (const auto& item : visible_)
    {
        const Rect popup = itemRect(item,screen,offset);
        if (popup.height <= 0) break;
        const Color accent = item.notification.severity == ToastSeverity::Success ? Color::rgb(85,200,125) :
            item.notification.severity == ToastSeverity::Warning ? Color::rgb(240,185,80) :
            item.notification.severity == ToastSeverity::Error ? Color::rgb(235,95,105) : palette.primary;
        const TextStyle body {.foreground = palette.foreground,.background = palette.surface};
        TextStyle highlight = body;
        highlight.foreground = accent;
        highlight.bold = true;
        painter.fillBackground(popup,palette.surface);
        if (popup.height >= 3) painter.border(popup,BorderStyle::Rounded,highlight);
        const int inset = popup.height >= 3 ? 2 : 0;
        const int y = popup.y+(popup.height >= 3 ? 1 : 0);
        const std::string_view icon = item.notification.severity == ToastSeverity::Success ? "✓" :
            item.notification.severity == ToastSeverity::Warning ? "!" :
            item.notification.severity == ToastSeverity::Error ? "×" : "i";
        painter.text(popup.x+inset,y,icon,highlight);
        painter.text(popup.x+inset+2,y,clipText(item.notification.message,
            std::max(0,popup.width-inset*2-2)),body);
        offset += popup.height+1;
    }
}
