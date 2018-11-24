// Copyright 2015 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <array>
#include <atomic>
#ifndef _MSC_VER
#include <cstddef>
#endif
#include <memory>
#include "common/bit_field.h"
#include "common/common_funcs.h"
#include "common/common_types.h"
#include "core/hle/kernel/kernel.h"
#include "core/hle/service/service.h"
#include "core/input.h"
#include "core/settings.h"

namespace Core {
class System;
struct TimingEventType;
} // namespace Core

namespace Kernel {
class SharedMemory;
} // namespace Kernel

namespace Service::HID {

/// Structure of a Pad controller state.
struct PadState {
    union {
        u32 hex{};

        BitField<0, 1, u32> a;
        BitField<1, 1, u32> b;
        BitField<2, 1, u32> select;
        BitField<3, 1, u32> start;
        BitField<4, 1, u32> right;
        BitField<5, 1, u32> left;
        BitField<6, 1, u32> up;
        BitField<7, 1, u32> down;
        BitField<8, 1, u32> r;
        BitField<9, 1, u32> l;
        BitField<10, 1, u32> x;
        BitField<11, 1, u32> y;

        BitField<28, 1, u32> circle_right;
        BitField<29, 1, u32> circle_left;
        BitField<30, 1, u32> circle_up;
        BitField<31, 1, u32> circle_down;
    };
};

/// Structure of a single entry of Pad state history within HID shared memory
struct PadDataEntry {
    PadState current_state;
    PadState delta_additions;
    PadState delta_removals;

    s16 circle_pad_x;
    s16 circle_pad_y;
};

/// Structure of a single entry of touch state history within HID shared memory
struct TouchDataEntry {
    u16 x;                     ///< Y-coordinate of a touchpad press on the lower screen
    u16 y;                     ///< X-coordinate of a touchpad press on the lower screen
    BitField<0, 7, u32> valid; ///< Set to 1 when this entry contains actual X/Y data, otherwise 0
};

/// Structure of a single entry of accelerometer state history within HID shared memory
struct AccelerometerDataEntry {
    s16 x;
    s16 y;
    s16 z;
};

/// Structure of a single entry of gyroscope state history within HID shared memory
struct GyroscopeDataEntry {
    s16 x;
    s16 y;
    s16 z;
};

/// Structure of data stored in HID shared memory
struct SharedMem {
    /// Pad data, this is used for buttons and the circle pad
    struct {
        s64 index_reset_ticks; ///< CPU tick count for when HID module updated entry index 0
        s64 index_reset_ticks_previous; ///< Previous `index_reset_ticks`
        u32 index;                      ///< Index of the last updated pad state entry

        INSERT_PADDING_WORDS(0x2);

        PadState current_state; ///< Current state of the pad buttons

        // TODO: Implement `raw_circle_pad_data` field
        u32 raw_circle_pad_data; ///< Raw (analog) circle pad data, before being converted

        INSERT_PADDING_WORDS(0x1);

        std::array<PadDataEntry, 8> entries; ///< Last 8 pad entries
    } pad;

    /// Touchpad data, this is used for touchpad input
    struct {
        s64 index_reset_ticks; ///< CPU tick count for when HID module updated entry index 0
        s64 index_reset_ticks_previous; ///< Previous `index_reset_ticks`
        u32 index;                      ///< Index of the last updated touch entry

        INSERT_PADDING_WORDS(0x1);

        // TODO: Implement `raw_entry` field
        TouchDataEntry raw_entry; ///< Raw (analog) touch data, before being converted

        std::array<TouchDataEntry, 8> entries; ///< Last 8 touch entries, in pixel coordinates
    } touch;

    /// Accelerometer data
    struct {
        s64 index_reset_ticks; ///< CPU tick count for when HID module updated entry index 0
        s64 index_reset_ticks_previous; ///< Previous `index_reset_ticks`
        u32 index;                      ///< Index of the last updated accelerometer entry

        INSERT_PADDING_WORDS(0x1);

        AccelerometerDataEntry raw_entry;
        INSERT_PADDING_BYTES(2);

        std::array<AccelerometerDataEntry, 8> entries;
    } accelerometer;

    /// Gyroscope data
    struct {
        s64 index_reset_ticks; ///< CPU tick count for when HID module updated entry index 0
        s64 index_reset_ticks_previous; ///< Previous `index_reset_ticks`
        u32 index;                      ///< Index of the last updated accelerometer entry

        INSERT_PADDING_WORDS(0x1);

        GyroscopeDataEntry raw_entry;
        INSERT_PADDING_BYTES(2);

        std::array<GyroscopeDataEntry, 32> entries;
    } gyroscope;
};

/// Structure of calibrate params that GetGyroscopeLowCalibrateParam returns
struct GyroscopeCalibrateParam {
    struct {
        // TODO: figure out the exact meaning of these params
        s16 zero_point;
        s16 positive_unit_point;
        s16 negative_unit_point;
    } x, y, z;
};

// TODO: MSVC doesn't support using offsetof() on non-static data members even though this
//       is technically allowed since C++11. This macro should be enabled once MSVC adds
//       support for that.
#ifndef _MSC_VER
#define ASSERT_REG_POSITION(field_name, position)                                                  \
    static_assert(offsetof(SharedMem, field_name) == position * 4,                                 \
                  "Field " #field_name " has invalid position")

ASSERT_REG_POSITION(pad.index_reset_ticks, 0x0);
ASSERT_REG_POSITION(touch.index_reset_ticks, 0x2A);

#undef ASSERT_REG_POSITION
#endif // !defined(_MSC_VER)

struct DirectionState {
    bool up;
    bool down;
    bool left;
    bool right;
};

/// Translates analog stick axes to directions. This is exposed for ir_rst module to use.
DirectionState GetStickDirectionState(s16 circle_pad_x, s16 circle_pad_y);

class Module final {
public:
    explicit Module(Core::System& system);

    class Interface : public ServiceFramework<Interface> {
    public:
        Interface(std::shared_ptr<Module> hid, const char* name);

        std::shared_ptr<Module> GetModule();

    protected:
        void GetIPCHandles(Kernel::HLERequestContext& ctx);
        void EnableAccelerometer(Kernel::HLERequestContext& ctx);
        void DisableAccelerometer(Kernel::HLERequestContext& ctx);
        void EnableGyroscopeLow(Kernel::HLERequestContext& ctx);
        void DisableGyroscopeLow(Kernel::HLERequestContext& ctx);
        void GetSoundVolume(Kernel::HLERequestContext& ctx);
        void GetGyroscopeLowRawToDpsCoefficient(Kernel::HLERequestContext& ctx);
        void GetGyroscopeLowCalibrateParam(Kernel::HLERequestContext& ctx);

        std::shared_ptr<Module> hid;
    };

    void ReloadInputDevices();
    void SetPadState(u32 raw);
    void SetTouchState(s16 x, s16 y, bool valid);
    void SetMotionState(s16 x, s16 y, s16 z, s16 roll, s16 pitch, s16 yaw);
    void SetCircleState(s16 x, s16 y);
    void SetOverrideControls(bool pad, bool touch, bool motion, bool circle);

    // The HID module of a console doesn't store the pad state.
    // Storing this here was necessary for emulation specific tasks like cheats or scripting.
    u32 pad_state{};

private:
    void LoadInputDevices();
    void UpdatePadCallback(u64 userdata, s64 cycles_late);
    void UpdateAccelerometerCallback(u64 userdata, s64 cycles_late);
    void UpdateGyroscopeCallback(u64 userdata, s64 cycles_late);

    // Handle to shared memory region designated to hid:USER service
    Kernel::SharedPtr<Kernel::SharedMemory> shared_mem;

    // Event handles
    Kernel::SharedPtr<Kernel::Event> event_pad_or_touch_1;
    Kernel::SharedPtr<Kernel::Event> event_pad_or_touch_2;
    Kernel::SharedPtr<Kernel::Event> event_accelerometer;
    Kernel::SharedPtr<Kernel::Event> event_gyroscope;
    Kernel::SharedPtr<Kernel::Event> event_debug_pad;

    u32 next_pad_index{};
    u32 next_touch_index{};
    u32 next_accelerometer_index{};
    u32 next_gyroscope_index{};

    int enable_accelerometer_count{}; // positive means enabled
    int enable_gyroscope_count{};     // positive means enabled

    Core::TimingEventType* pad_update_event;
    Core::TimingEventType* accelerometer_update_event;
    Core::TimingEventType* gyroscope_update_event;

    std::atomic_bool is_device_reload_pending{true};
    std::array<std::unique_ptr<Input::ButtonDevice>, Settings::NativeButton::NUM_BUTTONS_HID>
        buttons;
    std::unique_ptr<Input::ButtonDevice> button_home;
    std::unique_ptr<Input::AnalogDevice> circle_pad;
    std::unique_ptr<Input::MotionDevice> motion_device;
    std::unique_ptr<Input::TouchDevice> touch_device;

    bool use_override_pad_state{};
    bool use_override_touch{};
    bool use_override_motion{};
    bool use_override_circle_pad{};

    u32 override_pad_state{};
    s16 override_touch_x{};
    s16 override_touch_y{};
    bool override_touch_valid{};
    s16 override_motion_x{};
    s16 override_motion_y{};
    s16 override_motion_z{};
    s16 override_motion_roll{};
    s16 override_motion_pitch{};
    s16 override_motion_yaw{};
    s16 override_circle_x{};
    s16 override_circle_y{};

    Core::System& system;
};

void InstallInterfaces(Core::System& system);

} // namespace Service::HID
