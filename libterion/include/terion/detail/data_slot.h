/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */

#ifndef TERION_DETAIL_DATA_SLOT_H
#define TERION_DETAIL_DATA_SLOT_H

#include <memory>
#include <utility>

namespace terion::detail {

// Optional, owning component payload with value semantics.
// An absent slot consumes one pointer rather than reserving sizeof(T).
// Copying a component duplicates its payload; reconciliation may mutate copies safely.
template<typename T>
class DataSlot
{
public:
    DataSlot() noexcept = default;

    DataSlot(T value) : value_(std::make_unique<T>(std::move(value))) {}

    DataSlot(const DataSlot& other) :
        value_(other.value_ ? std::make_unique<T>(*other.value_) : nullptr)
    {}

    DataSlot(DataSlot&&) noexcept = default;

    DataSlot& operator=(DataSlot other) noexcept
    {
        value_.swap(other.value_);
        return *this;
    }

    [[nodiscard]] bool has_value() const noexcept { return static_cast<bool>(value_); }
    [[nodiscard]] explicit operator bool() const noexcept { return has_value(); }
    [[nodiscard]] const T& operator*() const noexcept { return *value_; }
    [[nodiscard]] T& operator*() noexcept { return *value_; }
    [[nodiscard]] const T* operator->() const noexcept { return value_.get(); }
    [[nodiscard]] T* operator->() noexcept { return value_.get(); }

    template<typename... Args>
    T& emplace(Args&&... args)
    {
        auto next = std::make_unique<T>(std::forward<Args>(args)...);
        value_ = std::move(next);
        return *value_;
    }

    void reset() noexcept { value_.reset(); }

private:
    std::unique_ptr<T> value_;
}; /* End of class DataSlot */

} /* End of namespace terion::detail */

#endif /* TERION_DETAIL_DATA_SLOT_H */
