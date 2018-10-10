// Copyright 2018 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <utility>
#include "core/frontend.h"

namespace Frontend {

static GetFramebufferLayoutFunction get_framebuffer_layout;
static NoReturnNoParametersFunction swap_buffers;
static NoReturnNoParametersFunction make_current;
static UpdateCurrentFramebufferLayoutFunction update_current_framebuffer_layout;

const Layout::FramebufferLayout& GetFramebufferLayout() {
    return get_framebuffer_layout();
}

void Set_GetFramebufferLayout(GetFramebufferLayoutFunction function) {
    get_framebuffer_layout = std::move(function);
}

void SwapBuffers() {
    swap_buffers();
}

void Set_SwapBuffers(NoReturnNoParametersFunction function) {
    swap_buffers = std::move(function);
}

void MakeCurrent() {
    make_current();
}

void Set_MakeCurrent(NoReturnNoParametersFunction function) {
    make_current = std::move(function);
}

void UpdateCurrentFramebufferLayout(unsigned int width, unsigned int height) {
    update_current_framebuffer_layout(width, height);
}

void Set_UpdateCurrentFramebufferLayout(UpdateCurrentFramebufferLayoutFunction function) {
    update_current_framebuffer_layout = std::move(function);
}

} // namespace Frontend