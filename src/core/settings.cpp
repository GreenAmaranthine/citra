// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "audio_core/hle/hle.h"
#include "core/core.h"
#include "core/hle/service/hid/hid.h"
#include "core/hle/service/ir/ir.h"
#include "core/hle/service/mic/mic_u.h"
#include "core/settings.h"
#include "video_core/renderer/renderer.h"
#include "video_core/video_core.h"

namespace Settings {

Values values{};

void Apply() {
    VideoCore::g_hw_shader_enabled = values.use_hw_shader;
    VideoCore::g_hw_shader_accurate_gs = values.shaders_accurate_gs;
    VideoCore::g_hw_shader_accurate_mul = values.shaders_accurate_mul;

    if (VideoCore::g_renderer) {
        VideoCore::g_renderer->UpdateCurrentFramebufferLayout();
        VideoCore::g_renderer->GetRasterizer()->SyncSettings();
    }

    VideoCore::g_renderer_bg_color_update_requested = true;

    if (Core::System::GetInstance().IsPoweredOn()) {
        Core::DSP().UpdateSink();
        Core::DSP().EnableStretching(values.enable_audio_stretching);
    }

    Service::HID::ReloadInputDevices();
    Service::IR::ReloadInputDevices();
    Service::CAM::ReloadCameraDevices();
    Service::MIC::ReloadDevice();
}

template <typename T>
void LogSetting(const std::string& name, const T& value) {
    LOG_INFO(Config, "{}: {}", name, value);
}

void LogSettings() {
    LOG_INFO(Config, "Citra Configuration:");
    LogSetting("ControlPanel_Volume", Settings::values.volume);
    LogSetting("ControlPanel_Factor3d", Settings::values.factor_3d);
    LogSetting("ControlPanel_HeadphonesConnected", Settings::values.headphones_connected);
    LogSetting("ControlPanel_AdapterConnected", Settings::values.p_adapter_connected);
    LogSetting("ControlPanel_BatteryCharging", Settings::values.p_battery_charging);
    LogSetting("ControlPanel_BatteryLevel", Settings::values.p_battery_level);
    LogSetting("ControlPanel_NetworkState", Settings::values.n_state);
    LogSetting("ControlPanel_WifiLinkLevel", Settings::values.n_wifi_link_level);
    LogSetting("ControlPanel_WifiStatus", Settings::values.n_wifi_status);
    LogSetting("Core_KeyboardMode", static_cast<int>(Settings::values.keyboard_mode));
    LogSetting("Core_EnableNSLaunch", Settings::values.enable_ns_launch);
    bool using_lle_modules{};
    for (const auto& module : Settings::values.lle_modules) {
        if (module.second) {
            using_lle_modules = true;
            break;
        }
    }
    LogSetting("LLE_UsingLLEModules", using_lle_modules);
    LogSetting("Renderer_UseHwShader", Settings::values.use_hw_shader);
    LogSetting("Renderer_ShadersAccurateGs", Settings::values.shaders_accurate_gs);
    LogSetting("Renderer_ShadersAccurateMul", Settings::values.shaders_accurate_mul);
    LogSetting("Renderer_UseResolutionFactor", Settings::values.resolution_factor);
    LogSetting("Renderer_UseFrameLimit", Settings::values.use_frame_limit);
    LogSetting("Renderer_FrameLimit", Settings::values.frame_limit);
    LogSetting("Renderer_EnableClearCache", Settings::values.enable_clear_cache);
    LogSetting("Renderer_ClearCacheMs", Settings::values.clear_cache_secs);
    LogSetting("Renderer_MinVerticesPerThread", Settings::values.min_vertices_per_thread);
    LogSetting("Layout_LayoutOption", static_cast<int>(Settings::values.layout_option));
    LogSetting("Layout_SwapScreen", Settings::values.swap_screen);
    LogSetting("Audio_EnableAudioStretching", Settings::values.enable_audio_stretching);
    LogSetting("Audio_OutputDevice", Settings::values.output_device);
    LogSetting("Audio_InputDevice", Settings::values.input_device);
    using namespace Service::CAM;
    LogSetting("Camera_OuterRightName", Settings::values.camera_name[OuterRightCamera]);
    LogSetting("Camera_OuterRightConfig", Settings::values.camera_config[OuterRightCamera]);
    LogSetting("Camera_OuterRightFlip", Settings::values.camera_flip[OuterRightCamera]);
    LogSetting("Camera_InnerName", Settings::values.camera_name[InnerCamera]);
    LogSetting("Camera_InnerConfig", Settings::values.camera_config[InnerCamera]);
    LogSetting("Camera_InnerFlip", Settings::values.camera_flip[InnerCamera]);
    LogSetting("Camera_OuterLeftName", Settings::values.camera_name[OuterLeftCamera]);
    LogSetting("Camera_OuterLeftConfig", Settings::values.camera_config[OuterLeftCamera]);
    LogSetting("Camera_OuterLeftFlip", Settings::values.camera_flip[OuterLeftCamera]);
    LogSetting("DataStorage_UseVirtualSd", Settings::values.use_virtual_sd);
    LogSetting("System_RegionValue", Settings::values.region_value);
    LogSetting("Hacks_PriorityBoost", Settings::values.priority_boost);
    LogSetting("Hacks_Ticks", Settings::values.ticks);
    LogSetting("Hacks_TicksMode", static_cast<int>(Settings::values.ticks_mode));
    LogSetting("Hacks_UseBos", Settings::values.use_bos);
    LogSetting("Hacks_DisableMh3d", Settings::values.disable_mh_3d);
}

void LoadProfile(int index) {
    const auto& profile{values.profiles.at(index)};
    values.profile = index;
    values.analogs = profile.analogs;
    values.buttons = profile.buttons;
    values.motion_device = profile.motion_device;
    values.touch_device = profile.touch_device;
    values.udp_input_address = profile.udp_input_address;
    values.udp_input_port = profile.udp_input_port;
    values.udp_pad_index = profile.udp_pad_index;
}

void SaveProfile(int index) {
    auto& profile{values.profiles.at(index)};
    profile.analogs = values.analogs;
    profile.buttons = values.buttons;
    profile.motion_device = values.motion_device;
    profile.touch_device = values.touch_device;
    profile.udp_input_address = values.udp_input_address;
    profile.udp_input_port = values.udp_input_port;
    profile.udp_pad_index = values.udp_pad_index;
}

void CreateProfile(std::string name) {
    ControllerProfile profile{};
    profile.name = name;
    profile.analogs = values.analogs;
    profile.buttons = values.buttons;
    profile.motion_device = values.motion_device;
    profile.touch_device = values.touch_device;
    profile.udp_input_address = values.udp_input_address;
    profile.udp_input_port = values.udp_input_port;
    profile.udp_pad_index = values.udp_pad_index;
    values.profiles.push_back(profile);
    LoadProfile(values.profiles.size() - 1);
}

void DeleteProfile(int index) {
    values.profiles.erase(values.profiles.begin() + index);
    LoadProfile(0);
}

} // namespace Settings
