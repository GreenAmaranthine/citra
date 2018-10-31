// Copyright 2018 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/3ds.h"
#include "core/frontend.h"
#include "core/input.h"
#include "core/settings.h"

class Frontend::TouchState : public Input::Factory<Input::TouchDevice>,
                             public std::enable_shared_from_this<TouchState> {
public:
    std::unique_ptr<Input::TouchDevice> Create(const Common::ParamPackage&) override {
        return std::make_unique<Device>(shared_from_this());
    }

    std::mutex mutex;

    bool touch_pressed{}; ///< True if touchpad area is currently pressed, otherwise false
    float touch_x{};      ///< Touchpad X-position
    float touch_y{};      ///< Touchpad Y-position

private:
    class Device : public Input::TouchDevice {
    public:
        explicit Device(std::weak_ptr<TouchState>&& touch_state) : touch_state{touch_state} {}
        std::tuple<float, float, bool> GetStatus() const override {
            if (auto state{touch_state.lock()}) {
                std::lock_guard<std::mutex> guard{state->mutex};
                return std::make_tuple(state->touch_x, state->touch_y, state->touch_pressed);
            }
            return std::make_tuple(0.0f, 0.0f, false);
        }

    private:
        std::weak_ptr<TouchState> touch_state;
    };
};

Frontend::Frontend() {
    touch_state = std::make_shared<TouchState>();
    Input::RegisterFactory<Input::TouchDevice>("emu_window", touch_state);
}

Frontend::~Frontend() {
    Input::UnregisterFactory<Input::TouchDevice>("emu_window");
}

/**
 * Check if the given x/y coordinates are within the touchpad specified by the framebuffer layout
 * @param layout FramebufferLayout object describing the framebuffer size and screen positions
 * @param framebuffer_x Framebuffer X-coordinate to check
 * @param framebuffer_y Framebuffer Y-coordinate to check
 * @return True if the coordinates are within the touchpad, otherwise false
 */
static bool IsWithinTouchscreen(const Layout::FramebufferLayout& layout, unsigned framebuffer_x,
                                unsigned framebuffer_y) {
    if (Settings::values.factor_3d != 0) {
        return (framebuffer_y >= layout.bottom_screen.top &&
                framebuffer_y < layout.bottom_screen.bottom &&
                framebuffer_x >= layout.bottom_screen.left / 2 &&
                framebuffer_x < layout.bottom_screen.right / 2);
    } else {
        return (framebuffer_y >= layout.bottom_screen.top &&
                framebuffer_y < layout.bottom_screen.bottom &&
                framebuffer_x >= layout.bottom_screen.left &&
                framebuffer_x < layout.bottom_screen.right);
    }
}

std::tuple<unsigned, unsigned> Frontend::ClipToTouchScreen(unsigned new_x, unsigned new_y) {
    new_x = std::max(new_x, framebuffer_layout.bottom_screen.left);
    new_x = std::min(new_x, framebuffer_layout.bottom_screen.right - 1);
    new_y = std::max(new_y, framebuffer_layout.bottom_screen.top);
    new_y = std::min(new_y, framebuffer_layout.bottom_screen.bottom - 1);
    return std::make_tuple(new_x, new_y);
}

std::tuple<unsigned, unsigned> Frontend::TouchPressed(unsigned framebuffer_x,
                                                      unsigned framebuffer_y) {
    if (!IsWithinTouchscreen(framebuffer_layout, framebuffer_x, framebuffer_y))
        return {};
    std::lock_guard<std::mutex> guard{touch_state->mutex};
    if (Settings::values.factor_3d != 0)
        touch_state->touch_x =
            static_cast<float>(framebuffer_x - framebuffer_layout.bottom_screen.left / 2) /
            (framebuffer_layout.bottom_screen.right / 2 -
             framebuffer_layout.bottom_screen.left / 2);
    else
        touch_state->touch_x =
            static_cast<float>(framebuffer_x - framebuffer_layout.bottom_screen.left) /
            (framebuffer_layout.bottom_screen.right - framebuffer_layout.bottom_screen.left);
    touch_state->touch_y =
        static_cast<float>(framebuffer_y - framebuffer_layout.bottom_screen.top) /
        (framebuffer_layout.bottom_screen.bottom - framebuffer_layout.bottom_screen.top);
    touch_state->touch_pressed = true;
    return {touch_state->touch_x * Core::kScreenBottomWidth,
            touch_state->touch_y * Core::kScreenBottomHeight};
}

void Frontend::TouchReleased() {
    std::lock_guard<std::mutex> guard{touch_state->mutex};
    touch_state->touch_pressed = false;
    touch_state->touch_x = 0;
    touch_state->touch_y = 0;
}

std::tuple<unsigned, unsigned> Frontend::TouchMoved(unsigned framebuffer_x,
                                                    unsigned framebuffer_y) {
    if (!touch_state->touch_pressed)
        return {};
    if (!IsWithinTouchscreen(framebuffer_layout, framebuffer_x, framebuffer_y))
        std::tie(framebuffer_x, framebuffer_y) = ClipToTouchScreen(framebuffer_x, framebuffer_y);
    return TouchPressed(framebuffer_x, framebuffer_y);
}

void Frontend::UpdateCurrentFramebufferLayout(unsigned width, unsigned height) {
    Layout::FramebufferLayout layout;
    if (Settings::values.custom_layout) {
        layout = Layout::CustomFrameLayout(width, height, Settings::values.swap_screen);
    } else {
        switch (Settings::values.layout_option) {
        case Settings::LayoutOption::SingleScreen:
            layout = Layout::SingleFrameLayout(width, height, Settings::values.swap_screen);
            break;
        case Settings::LayoutOption::MediumScreen:
            layout = Layout::MediumFrameLayout(width, height, Settings::values.swap_screen);
            break;
        case Settings::LayoutOption::LargeScreen:
            layout = Layout::LargeFrameLayout(width, height, Settings::values.swap_screen);
            break;
        case Settings::LayoutOption::SideScreen:
            layout = Layout::SideFrameLayout(width, height, Settings::values.swap_screen);
            break;
        case Settings::LayoutOption::Default:
        default:
            layout = Layout::DefaultFrameLayout(width, height, Settings::values.swap_screen);
            break;
        }
    }
    framebuffer_layout = std::move(layout);
}
