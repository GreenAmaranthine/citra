// Copyright 2018 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include <tuple>
#include "core/framebuffer_layout.h"
#include "core/hle/applets/erreula.h"
#include "core/hle/applets/mii_selector.h"
#include "core/hle/applets/swkbd.h"

class Frontend {
public:
    Frontend();
    virtual ~Frontend();

    void UpdateCurrentFramebufferLayout(unsigned width, unsigned height);

    std::tuple<unsigned, unsigned> TouchPressed(unsigned framebuffer_x, unsigned framebuffer_y);
    void TouchReleased();
    std::tuple<unsigned, unsigned> TouchMoved(unsigned framebuffer_x, unsigned framebuffer_y);
    std::tuple<unsigned, unsigned> ClipToTouchScreen(unsigned new_x, unsigned new_y);

    const Layout::FramebufferLayout& GetFramebufferLayout() {
        return framebuffer_layout;
    }

    virtual void SwapBuffers() = 0;
    virtual void MakeCurrent() = 0;
    virtual void DoneCurrent() = 0;

    virtual void LaunchSoftwareKeyboard(HLE::Applets::SoftwareKeyboardConfig&, std::u16string&,
                                        bool&) = 0;
    virtual void LaunchErrEula(HLE::Applets::ErrEulaConfig&, bool&) = 0;
    virtual void LaunchMiiSelector(const HLE::Applets::MiiConfig&, HLE::Applets::MiiResult&,
                                   bool&) = 0;
    virtual void Update3D() {}
    virtual void UpdateNetwork() {}
    virtual void UpdateFrameAdvancing() {}

private:
    Layout::FramebufferLayout framebuffer_layout; ///< Current framebuffer layout

    class TouchState;
    std::shared_ptr<TouchState> touch_state;
};
