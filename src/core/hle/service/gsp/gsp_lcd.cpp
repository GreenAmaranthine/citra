// Copyright 2015 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <cstdlib>
#include <sstream>
#include "core/hle/ipc_helpers.h"
#include "core/hle/service/gsp/gsp_lcd.h"

namespace Service::GSP {

constexpr u32 MIN_BRIGHTNESS_RAW{0x10};
constexpr u32 MAX_BRIGHTNESS_RAW{0xAC};
constexpr u32 MIN_BRIGHTNESS{1};
constexpr u32 MAX_BRIGHTNESS{5};

void GSP_LCD::SetBrightnessRaw(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx, 0xA, 2, 0};
    u32 screen{rp.Pop<u32>()};
    u32 brightness{std::clamp(rp.Pop<u32>(), MIN_BRIGHTNESS_RAW, MAX_BRIGHTNESS_RAW)};
    float brightness_f{static_cast<float>(brightness / MAX_BRIGHTNESS_RAW)};
    int ret{1};
    // TODO: add support for Windows & macOS
#ifdef __linux__
    std::string command{"xrandr --output eDP-1 --brightness "};
    std::ostringstream os;
    os << brightness_f;
    command += os.str();
    ret = system(command.c_str());
#endif
    if (ret == 0)
        this->brightness = brightness_f;
    IPC::ResponseBuilder rb{rp.MakeBuilder(1, 0)};
    rb.Push(RESULT_SUCCESS);
}

void GSP_LCD::SetBrightness(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx, 0xB, 2, 0};
    u32 screen{rp.Pop<u32>()};
    u32 brightness{std::clamp(rp.Pop<u32>(), MIN_BRIGHTNESS, MAX_BRIGHTNESS)};
    float brightness_f{static_cast<float>(brightness / MAX_BRIGHTNESS)};
    int ret{1};
    // TODO: add support for Windows & macOS
#ifdef __linux__
    std::string command{"xrandr --output eDP-1 --brightness "};
    std::ostringstream os;
    os << brightness_f;
    command += os.str();
    ret = system(command.c_str());
#endif
    if (ret == 0)
        this->brightness = brightness_f;
    IPC::ResponseBuilder rb{rp.MakeBuilder(1, 0)};
    rb.Push(RESULT_SUCCESS);
}

void GSP_LCD::GetBrightness(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx, 0x15, 1, 0};
    u32 screen{rp.Pop<u32>()};
    IPC::ResponseBuilder rb{rp.MakeBuilder(2, 0)};
    rb.Push(RESULT_SUCCESS);
    rb.Push<u32>(static_cast<u32>(brightness * MAX_BRIGHTNESS_RAW));
}

GSP_LCD::GSP_LCD() : ServiceFramework{"gsp::Lcd"} {
    static const FunctionInfo functions[]{
        {0x000A0080, &GSP_LCD::SetBrightnessRaw, "SetBrightnessRaw"},
        {0x000B0080, &GSP_LCD::SetBrightness, "SetBrightness"},
        {0x000F0000, nullptr, "PowerOnAllBacklights"},
        {0x00100000, nullptr, "PowerOffAllBacklights"},
        {0x00110040, nullptr, "PowerOnBacklight"},
        {0x00120040, nullptr, "PowerOffBacklight"},
        {0x00130040, nullptr, "SetLedForceOff"},
        {0x00140000, nullptr, "GetVendor"},
        {0x00150040, &GSP_LCD::GetBrightness, "GetBrightness"},
    };
    RegisterHandlers(functions);
    // TODO: get brightness & update it in Citra when the user changes it
};

} // namespace Service::GSP
