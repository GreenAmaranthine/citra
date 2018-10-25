// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <cstring>
#include "core/hle/kernel/config_mem.h"

namespace ConfigMem {

ConfigMemDef config_mem;

ConfigMemDef::ConfigMemDef() {
    std::memset(this, 0, sizeof(ConfigMemDef));

    // TODO: Update this
    // Values extracted from firmware 11.2.0-35E
    kernel_version_min = 0x34;
    kernel_version_maj = 0x2;
    ns_tid = 0x0004013000008002;
    sys_core_ver = 0x2;
    unit_info = 0x1; // Bit 0 set for Retail
    prev_firm = 0x1;
    ctr_sdk_ver = 0x0000F297;
    firm_version_min = 0x34;
    firm_version_maj = 0x2;
    firm_sys_core_ver = 0x2;
    firm_ctr_sdk_ver = 0x0000F297;
}

} // namespace ConfigMem
