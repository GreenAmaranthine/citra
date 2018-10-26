// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include "common/common_types.h"
#include "core/hle/kernel/process.h"

namespace Kernel {

class VMManager;

void HandleSpecialMapping(VMManager& address_space, const AddressMapping& mapping);

} // namespace Kernel
