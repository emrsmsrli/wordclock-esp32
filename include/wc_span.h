// Copyright (C) 2023 Emre Simsirli

#ifndef WORDCLOCK_WC_SPAN_H
#define WORDCLOCK_WC_SPAN_H

#include <cstdint>

namespace wordclock {

template<typename T>
class span {
    T* data_ = nullptr;
    std::size_t size_ = 0;

public:
    span() noexcept = default;
    span(T* data, std::size_t size) noexcept : data_(data), size_(size) {}

    T* begin() noexcept { return data_; }
    const T* begin() const noexcept { return data_; }
    const T* cbegin() const noexcept { return data_; }

    T* end() noexcept { return data_ + size_; }
    const T* end() const noexcept { return data_ + size_; }
    const T* cend() const noexcept { return data_ + size_; }

    T& operator[](const std::size_t idx) noexcept { return *(data_ + idx); }
    const T& operator[](const std::size_t idx) const noexcept { return *(data_ + idx); }
};

} // namespace wordclock

#endif //WORDCLOCK_WC_SPAN_H
