// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <atomic>
#include <memory>
#include "core/core.h"
#include "core/framebuffer_layout.h"

namespace Core {
class System;
} // namespace Core

class Renderer;

namespace VideoCore {

extern std::unique_ptr<Renderer> g_renderer;
extern std::atomic_bool g_hw_shaders_enabled;
extern std::atomic_bool g_hw_shaders_accurate_gs;
extern std::atomic_bool g_hw_shaders_accurate_mul;
extern std::atomic_bool g_bg_color_update_requested;
extern std::atomic_bool g_screenshot_requested;
extern void* g_screenshot_bits;
extern std::function<void()> g_screenshot_complete_callback;
extern Layout::FramebufferLayout g_screenshot_framebuffer_layout;

/// Initialize the video core
Core::System::ResultStatus Init(Core::System& system);

/// Shutdown the video core
void Shutdown();

/// Request a screenshot of the next frame
void RequestScreenshot(void* data, std::function<void()> callback,
                       const Layout::FramebufferLayout& layout);

} // namespace VideoCore
