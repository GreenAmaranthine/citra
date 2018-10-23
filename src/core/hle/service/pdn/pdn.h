// Copyright 2018 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "core/hle/service/service.h"

namespace Core {
class System;
} // namespace Core

namespace Service::PDN {

void InstallInterfaces(Core::System& system);

} // namespace Service::PDN
