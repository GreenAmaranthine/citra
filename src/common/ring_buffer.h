// Copyright 2018 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <algorithm>
#include <array>
#include <atomic>
#include <cstddef>
#include <cstring>
#include <type_traits>
#include <vector>
#include "common/common_types.h"

namespace Common {

/// SPSC ring buffer
/// @tparam T            Element type
/// @tparam capacity     Number of slots in ring buffer
/// @tparam granularity  Slot size in terms of number of elements
template <typename T, size_t capacity, size_t granularity = 1>
class RingBuffer {
    /// A "slot" is made of `granularity` elements of `T`.
    static constexpr size_t slot_size = granularity * sizeof(T);
    // T must be safely memcpy-able and have a trivial default constructor.
    static_assert(std::is_trivial_v<T>);
    // Ensure capacity is sensible.
    static_assert(capacity < std::numeric_limits<size_t>::max() / 2 / granularity);
    static_assert((capacity & (capacity - 1)) == 0, "capacity must be a power of two");
    // Ensure lock-free.
    static_assert(std::atomic<size_t>::is_always_lock_free);

public:
    /// Pushes slots into the ring buffer
    /// @param new_slots   Pointer to the slots to push
    /// @param slot_count  Number of slots to push
    /// @returns The number of slots actually pushed
    size_t Push(const void* new_slots, size_t slot_count) {
        const size_t write_index{m_write_index.load()};
        const size_t slots_free{capacity + m_read_index.load() - write_index};
        const size_t push_count{std::min(slot_count, slots_free)};

        const size_t pos{write_index % capacity};
        const size_t first_copy{std::min(capacity - pos, push_count)};
        const size_t second_copy{push_count - first_copy};

        const char* in{static_cast<const char*>(new_slots)};
        std::memcpy(m_data.data() + pos * granularity, in, first_copy * slot_size);
        in += first_copy * slot_size;
        std::memcpy(m_data.data(), in, second_copy * slot_size);

        m_write_index.store(write_index + push_count);

        return push_count;
    }

    size_t Push(const std::vector<T>& input) {
        return Push(input.data(), input.size());
    }

    /// Pops slots from the ring buffer
    /// @param output     Where to store the popped slots
    /// @param max_slots  Maximum number of slots to pop
    /// @returns The number of slots actually popped
    size_t Pop(void* output, size_t max_slots = ~size_t(0)) {
        const size_t read_index{m_read_index.load()};
        const size_t slots_filled{m_write_index.load() - read_index};
        const size_t pop_count{std::min(slots_filled, max_slots)};

        const size_t pos{read_index % capacity};
        const size_t first_copy{std::min(capacity - pos, pop_count)};
        const size_t second_copy{pop_count - first_copy};

        char* out{static_cast<char*>(output)};
        std::memcpy(out, m_data.data() + pos * granularity, first_copy * slot_size);
        out += first_copy * slot_size;
        std::memcpy(out, m_data.data(), second_copy * slot_size);

        m_read_index.store(read_index + pop_count);

        return pop_count;
    }

    std::vector<T> Pop(size_t max_slots = ~size_t(0)) {
        std::vector<T> out(std::min(max_slots, capacity) * granularity);
        const size_t count{Pop(out.data(), out.size() / granularity)};
        out.resize(count * granularity);
        return out;
    }

    /// @returns Number of slots used
    size_t Size() const {
        return m_write_index.load() - m_read_index.load();
    }

    /// @returns Maximum size of ring buffer
    constexpr size_t Capacity() const {
        return capacity;
    }

private:
    // It is important to align the below variables for performance reasons:
    // Having them on the same cache-line would result in false-sharing between them.
    alignas(128) std::atomic<size_t> m_read_index{0};
    alignas(128) std::atomic<size_t> m_write_index{0};

    std::array<T, granularity * capacity> m_data;
};

} // namespace Common