// Copyright 2015 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "core/hle/kernel/kernel.h"
#include "core/hle/service/service.h"

namespace Core {
class System;
} // namespace Core

namespace Service::NS {

/// Interface to "ns:s" service
class NS_S final : public ServiceFramework<NS_S> {
public:
    explicit NS_S(Core::System& system);
    ~NS_S();

    void LaunchTitle(Kernel::HLERequestContext& ctx);
    void ShutdownAsync(Kernel::HLERequestContext& ctx);
    void RebootSystemClean(Kernel::HLERequestContext& ctx);

private:
    Core::System& system;
};

} // namespace Service::NS
