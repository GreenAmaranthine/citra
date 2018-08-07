// This file is under the public domain.

#pragma once

#include <cstddef>
#include <type_traits>

namespace Common {

template <typename T>
<<<<<<< HEAD
constexpr T AlignUp(T value, std::size_t size) {
    static_assert(std::is_unsigned<T>::value, "T must be an unsigned value.");
=======
constexpr T AlignUp(T value, size_t size) {
    static_assert(std::is_unsigned_v<T>, "T must be an unsigned value.");
>>>>>>> bf964ac6e... common: Convert type traits templates over to variable template versions where applicable
    return static_cast<T>(value + (size - value % size) % size);
}

template <typename T>
<<<<<<< HEAD
constexpr T AlignDown(T value, std::size_t size) {
    static_assert(std::is_unsigned<T>::value, "T must be an unsigned value.");
=======
constexpr T AlignDown(T value, size_t size) {
    static_assert(std::is_unsigned_v<T>, "T must be an unsigned value.");
>>>>>>> bf964ac6e... common: Convert type traits templates over to variable template versions where applicable
    return static_cast<T>(value - value % size);
}

} // namespace Common
