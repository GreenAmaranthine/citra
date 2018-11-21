// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "core/hle/kernel/process.h"
#include "core/hle/service/fs/archive.h"
#include "core/hle/service/service.h"

namespace Core {
class System;
} // namespace Core

namespace Service::NS {

/// Loads and launches the program identified by program_id in the specified media type.
Kernel::SharedPtr<Kernel::Process> Launch(Core::System& system, FS::MediaType media_type,
                                          u64 program_id);

/// Registers all NS services with the specified service manager.
void InstallInterfaces(Core::System& system);

} // namespace Service::NS
