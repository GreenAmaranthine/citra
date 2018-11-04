// Copyright 2018 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <array>
#include "common/common_types.h"

#define USER32MODE 16
#define FPSCR_DEFAULT_NAN (1 << 25)
#define FPSCR_FLUSH_TO_ZERO (1 << 24)
#define FPSCR_ROUND_TOZERO (3 << 22)
#define FPSCR_IXC (1 << 4)

enum VFPSystemRegister {
    VFP_FPSCR = 1,
    VFP_FPEXC = 2,
};

enum CP15Register {
    CP15_FLUSH_PREFETCH_BUFFER = 36,
    CP15_DATA_SYNC_BARRIER = 46,
    CP15_DATA_MEMORY_BARRIER = 47,
    CP15_THREAD_UPRW = 69,
    CP15_THREAD_URO = 70,
};

struct State {
    std::array<u32, 84> cp15{};
    std::array<u32, 7> vfp{};
};
