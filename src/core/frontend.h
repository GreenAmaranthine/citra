// Copyright 2018 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <functional>
#include "core/framebuffer_layout.h"

namespace Frontend {

using GetFramebufferLayoutFunction = std::function<const Layout::FramebufferLayout&()>;
using NoReturnNoParametersFunction = std::function<void()>;
using UpdateCurrentFramebufferLayoutFunction = std::function<void(unsigned int, unsigned int)>;

const Layout::FramebufferLayout& GetFramebufferLayout();
void Set_GetFramebufferLayout(GetFramebufferLayoutFunction function);

void SwapBuffers();
void Set_SwapBuffers(NoReturnNoParametersFunction function);

void MakeCurrent();
void Set_MakeCurrent(NoReturnNoParametersFunction function);

void UpdateCurrentFramebufferLayout(unsigned int width, unsigned int height);
void Set_UpdateCurrentFramebufferLayout(UpdateCurrentFramebufferLayoutFunction function);

} // namespace Frontend
