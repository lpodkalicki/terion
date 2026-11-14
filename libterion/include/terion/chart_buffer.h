/*
 * Copyright (c) 2026 by Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 */
#ifndef TERION_CHART_BUFFER_H
#define TERION_CHART_BUFFER_H

#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <utility>
#include <vector>

namespace terion {

// Bounded, single-threaded sample history. Indices are stable as old samples expire.
class ChartBuffer
{
public:
    explicit ChartBuffer(std::size_t capacity, std::size_t channels) :
        capacity_(capacity), values_(channels,std::vector<double>(capacity))
    {
        if (!capacity || !channels) throw std::invalid_argument("ChartBuffer requires capacity and channels");
    }

    void append(const std::vector<double>& sample)
    {
        if (sample.size() != values_.size()) throw std::invalid_argument("ChartBuffer channel count mismatch");
        const auto position = (head_+size_)%capacity_;
        for (std::size_t channel=0;channel<values_.size();++channel)
            values_[channel][position] = sample[channel];
        if (size_ < capacity_) ++size_;
        else { head_=(head_+1)%capacity_; ++first_index_; }
    }

    [[nodiscard]] std::size_t size() const noexcept { return size_; }
    [[nodiscard]] std::size_t capacity() const noexcept { return capacity_; }
    [[nodiscard]] std::size_t channels() const noexcept { return values_.size(); }
    [[nodiscard]] std::size_t beginIndex() const noexcept { return first_index_; }
    [[nodiscard]] std::size_t endIndex() const noexcept { return first_index_+size_; }
    [[nodiscard]] std::vector<double> channel(std::size_t index) const
    {
        if (index >= values_.size()) throw std::out_of_range("ChartBuffer channel");
        std::vector<double> result;
        result.reserve(size_);
        for (std::size_t i=0;i<size_;++i)
            result.push_back(values_[index][(head_+i)%capacity_]);
        return result;
    }

    void clear() noexcept { head_=0; size_=0; first_index_=0; }

private:
    std::size_t capacity_;
    std::vector<std::vector<double>> values_;
    std::size_t head_=0;
    std::size_t size_=0;
    std::size_t first_index_=0;
}; /* End of class ChartBuffer */

} /* End of namespace terion */
#endif /* TERION_CHART_BUFFER_H */
