// Copyright 2016 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "core/hle/service/service.h"

namespace Core {
class System;
} // namespace Core

namespace SharedPage {
class Handler;
} // namespace SharedPage

namespace Kernel {
class HLERequestContext;
} // namespace Kernel

namespace Service::NWM {

class NWM_EXT final : public ServiceFramework<NWM_EXT> {
public:
    explicit NWM_EXT(Core::System& system);
    ~NWM_EXT();

private:
    void ControlWirelessEnabled(Kernel::HLERequestContext& ctx);

    Core::System& system;
};

} // namespace Service::NWM
