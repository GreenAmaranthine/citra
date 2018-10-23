// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "core/hle/service/service.h"

namespace Core {
class System;
} // namespace Core

namespace Kernel {
class HLERequestContext;
} // namespace Kernel

namespace Service::ERR {

/// Interface to "err:f" service
class ERR_F final : public ServiceFramework<ERR_F> {
public:
    explicit ERR_F(Core::System& system);
    ~ERR_F();

private:
    void ThrowFatalError(Kernel::HLERequestContext& ctx);
    void SetUserString(Kernel::HLERequestContext& ctx);

    Core::System& system;
};

void InstallInterfaces(Core::System& system);

} // namespace Service::ERR
