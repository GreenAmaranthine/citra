// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "core/hle/kernel/mutex.h"
#include "core/hle/kernel/shared_memory.h"
#include "core/hle/service/service.h"

namespace Core {
class System;
} // namespace Core

namespace Service::CSND {

class CSND_SND final : public ServiceFramework<CSND_SND> {
public:
    CSND_SND();
    ~CSND_SND() = default;

private:
    void Initialize(Kernel::HLERequestContext& ctx);
    void Shutdown(Kernel::HLERequestContext& ctx);
    void ExecuteCommands(Kernel::HLERequestContext& ctx);
    void ExecuteType1Commands(Kernel::HLERequestContext& ctx);
    void AcquireSoundChannels(Kernel::HLERequestContext& ctx);
    void ReleaseSoundChannels(Kernel::HLERequestContext& ctx);
    void AcquireCapUnit(Kernel::HLERequestContext& ctx);
    void ReleaseCapUnit(Kernel::HLERequestContext& ctx);
    void FlushDataCache(Kernel::HLERequestContext& ctx);
    void StoreDataCache(Kernel::HLERequestContext& ctx);
    void InvalidateDataCache(Kernel::HLERequestContext& ctx);
    void Reset(Kernel::HLERequestContext& ctx);

    struct Type0Command {
        // Command id and next command offset
        u32 command_id;
        u32 finished;
        u32 flags;
        u8 parameters[20];
    };
    static_assert(sizeof(Type0Command) == 0x20, "Type0Command structure size is wrong");

    Kernel::SharedPtr<Kernel::Mutex> mutex;
    Kernel::SharedPtr<Kernel::SharedMemory> shared_memory;

    static constexpr u32 MaxCaptureUnits{2};
    std::array<bool, MaxCaptureUnits> capture_units{false, false};
};

/// Initializes the CSND_SND Service
void InstallInterfaces(Core::System& system);

} // namespace Service::CSND
