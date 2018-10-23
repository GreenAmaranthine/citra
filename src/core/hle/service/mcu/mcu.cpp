// Copyright 2018 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <SDL.h>
#include "common/logging/log.h"
#include "core/core.h"
#include "core/hle/ipc.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/result.h"
#include "core/hle/service/mcu/cam.h"
#include "core/hle/service/mcu/cdc.h"
#include "core/hle/service/mcu/gpu.h"
#include "core/hle/service/mcu/hid.h"
#include "core/hle/service/mcu/hwc.h"
#include "core/hle/service/mcu/mcu.h"
#include "core/hle/service/mcu/nwm.h"
#include "core/hle/service/mcu/pls.h"
#include "core/hle/service/mcu/rtc.h"
#include "core/hle/service/mcu/snd.h"
#include "core/settings.h"

namespace Service::MCU {

void Module::Interface::GetBatteryLevel(Kernel::HLERequestContext& ctx, u16 id) {
    IPC::ResponseBuilder rb{ctx, id, 2, 0};
    rb.Push(RESULT_SUCCESS);
    int pct;
    SDL_GetPowerInfo(nullptr, &pct);
    rb.Push<u8>(static_cast<u8>(pct == -1 ? 0x64 : pct));
}

void Module::Interface::GetBatteryChargeState(Kernel::HLERequestContext& ctx) {
    IPC::ResponseBuilder rb{ctx, 0x2C, 2, 0};
    rb.Push(RESULT_SUCCESS);
    rb.Push(Settings::values.p_battery_charging);
}

void Module::Interface::Set3DLEDState(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx, 0x0009, 1, 0};
    u8 state{rp.Pop<u8>()};
    if (!Settings::values.disable_mh_3d) {
        if (state == 0)
            Settings::values.factor_3d = 0;
        else
            Settings::values.factor_3d = 100;
        mcu->system.GetSharedPageHandler()->Update3DSettings();
    }
    IPC::ResponseBuilder rb{rp.MakeBuilder(1, 0)};
    rb.Push(RESULT_SUCCESS);
}

void Module::Interface::GetSoundVolume(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx, IPC::Header{ctx.CommandBuffer()[0]}};
    IPC::ResponseBuilder rb{rp.MakeBuilder(2, 0)};
    rb.Push(RESULT_SUCCESS);
    rb.Push<u8>(static_cast<u8>(0x3F * Settings::values.volume));
}

Module::Interface::Interface(std::shared_ptr<Module> mcu, const char* name)
    : ServiceFramework{name, 1}, mcu{std::move(mcu)} {}

Module::Module(Core::System& system) : system{system} {}

void InstallInterfaces(Core::System& system) {
    auto& service_manager{system.ServiceManager()};
    auto mcu{std::make_shared<Module>(system)};
    std::make_shared<CAM>(mcu)->InstallAsService(service_manager);
    std::make_shared<CDC>(mcu)->InstallAsService(service_manager);
    std::make_shared<GPU>(mcu)->InstallAsService(service_manager);
    std::make_shared<HID>(mcu)->InstallAsService(service_manager);
    std::make_shared<HWC>(mcu)->InstallAsService(service_manager);
    std::make_shared<NWM>(mcu)->InstallAsService(service_manager);
    std::make_shared<PLS>(mcu)->InstallAsService(service_manager);
    std::make_shared<RTC>(mcu)->InstallAsService(service_manager);
    std::make_shared<SND>(mcu)->InstallAsService(service_manager);
}

} // namespace Service::MCU
