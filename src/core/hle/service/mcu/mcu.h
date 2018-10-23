// Copyright 2018 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include "core/hle/kernel/kernel.h"
#include "core/hle/service/service.h"

namespace Core {
class System;
} // namespace Core

namespace Service::MCU {

class Module final {
public:
    explicit Module(Core::System& system);

    class Interface : public ServiceFramework<Interface> {
    public:
        explicit Interface(std::shared_ptr<Module> mcu, const char* name);

        void GetBatteryLevel(Kernel::HLERequestContext& ctx, u16 id);
        void GetBatteryChargeState(Kernel::HLERequestContext& ctx);
        void Set3DLEDState(Kernel::HLERequestContext& ctx);
        void GetSoundVolume(Kernel::HLERequestContext& ctx);

        template <void (Interface::*function)(Kernel::HLERequestContext& ctx, u16 id), u16 id>
        void D(Kernel::HLERequestContext& ctx) {
            (this->*function)(ctx, id);
        }

    protected:
        std::shared_ptr<Module> mcu;
    };

private:
    Core::System& system;
};

void InstallInterfaces(Core::System& system);

} // namespace Service::MCU
