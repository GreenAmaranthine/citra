// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "core/input.h"

namespace InputCommon {

class MotionEmuDevice;

class MotionEmu : public Input::Factory<Input::MotionDevice> {
public:
    /**
     * Creates a motion device emulated from mouse input
     * @param params contains parameters for creating the device:
     *     - "update_period": update period in milliseconds
     *     - "sensitivity": the coefficient converting mouse movement to tilting angle
     */
    std::unique_ptr<Input::MotionDevice> Create(const Common::ParamPackage& params) override;

    /**
     * Signals that a motion sensor tilt has begun.
     * @param x The X-coordinate of the cursor
     * @param y The Y-coordinate of the cursor
     */
    void BeginTilt(int x, int y);

    /**
     * Signals that a motion sensor tilt is occurring.
     * @param x The X-coordinate of the cursor
     * @param y The Y-coordinate of the cursor
     */
    void Tilt(int x, int y);

    /**
     * Signals that a motion sensor tilt has ended.
     */
    void EndTilt();

private:
    std::weak_ptr<MotionEmuDevice> current_device;
};

} // namespace InputCommon
