/**
 * Copyright (C) 2005-2012 Gekko Emulator
 *
 * @file    common_types.h
 * @author  ShizZy <shizzy247@gmail.com>
 * @date    2012-02-11
 * @brief   Common types used throughout the project
 *
 * @section LICENSE
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details at
 * http://www.gnu.org/copyleft/gpl.html
 *
 * Official project repository can be found at:
 * http://code.google.com/p/gekko-gc-emu/
 */

#pragma once

#include <array>
#include <cstdint>

#ifdef _MSC_VER
#ifndef __func__
#define __func__ __FUNCTION__
#endif
#endif

using u8 = std::uint8_t;   ///< 8-bit unsigned byte
using u16 = std::uint16_t; ///< 16-bit unsigned short
using u32 = std::uint32_t; ///< 32-bit unsigned word
using u64 = std::uint64_t; ///< 64-bit unsigned int
using s8 = std::int8_t;    ///< 8-bit signed byte
using s16 = std::int16_t;  ///< 16-bit signed short
using s32 = std::int32_t;  ///< 32-bit signed word
using s64 = std::int64_t;  ///< 64-bit signed int
using f32 = float;         ///< 32-bit floating point

// TODO: It would be nice to eventually replace these with strong types that prevent accidental
// conversion between each other.
using VAddr = u32; ///< Represents a pointer in the userspace virtual address space.
using PAddr = u32; ///< Represents a pointer in the ARM11 physical address space.

using MACAddress = std::array<u8, 6>; ///< Network MAC address

constexpr MACAddress NintendoOUI{0x00, 0x1F, 0x32, 0x00, 0x00, 0x00};
constexpr std::array<u8, 3> NintendoOUI3{0x00, 0x1F, 0x32};

constexpr MACAddress BroadcastMac{0xFF, 0xFF, 0xFF,
                                  0xFF, 0xFF, 0xFF}; ///< 802.11 broadcast MAC address

// An inheritable class to disallow the copy constructor and operator= functions
class NonCopyable {
protected:
    constexpr NonCopyable() = default;
    ~NonCopyable() = default;
    NonCopyable(const NonCopyable&) = delete;
    NonCopyable& operator=(const NonCopyable&) = delete;
};
