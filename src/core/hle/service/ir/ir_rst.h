// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included..

#pragma once

#include <atomic>
#include <memory>
#include "common/bit_field.h"
#include "common/common_types.h"
#include "common/swap.h"
#include "core/hle/kernel/kernel.h"
#include "core/hle/service/service.h"
#include "core/input.h"

namespace Core {
struct TimingEventType;
} // namespace Core

namespace Kernel {
class Event;
class SharedMemory;
} // namespace Kernel

namespace Service::IR {

union PadState {
    u32_le hex{};

    BitField<14, 1, u32_le> zl;
    BitField<15, 1, u32_le> zr;

    BitField<24, 1, u32_le> c_stick_right;
    BitField<25, 1, u32_le> c_stick_left;
    BitField<26, 1, u32_le> c_stick_up;
    BitField<27, 1, u32_le> c_stick_down;
};

/// Interface to "ir:rst" service
class IR_RST final : public ServiceFramework<IR_RST> {
public:
    explicit IR_RST(Core::System& system);
    ~IR_RST();
    void ReloadInputDevices();

private:
    void GetHandles(Kernel::HLERequestContext& ctx);
    void Initialize(Kernel::HLERequestContext& ctx);
    void Shutdown(Kernel::HLERequestContext& ctx);

    void LoadInputDevices();
    void UnloadInputDevices();
    void UpdateCallback(u64 userdata, s64 cycles_late);

    Core::System& system;
    Kernel::SharedPtr<Kernel::Event> update_event;
    Kernel::SharedPtr<Kernel::SharedMemory> shared_memory;
    u32 next_pad_index{};
    Core::TimingEventType* update_callback_id;
    std::unique_ptr<Input::ButtonDevice> zr_button;
    std::unique_ptr<Input::AnalogDevice> c_stick;
    std::atomic_bool is_device_reload_pending{};
    bool raw_c_stick{};
    int update_period{};
};

} // namespace Service::IR
