// Copyright 2018 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <string>
#include "core/camera/factory.h"

namespace Camera {

// Base class for Qt camera interfaces
class QtCameraInterface : public CameraInterface {
public:
    QtCameraInterface(const Service::CAM::Flip& flip);
    void SetResolution(const Service::CAM::Resolution&) override;
    void SetFlip(Service::CAM::Flip) override;
    void SetEffect(Service::CAM::Effect) override;
    void SetFormat(Service::CAM::OutputFormat) override;
    std::vector<u16> ReceiveFrame() override;
    virtual QImage QtReceiveFrame() = 0;

private:
    int width, height;
    bool output_rgb, flip_horizontal, flip_vertical, basic_flip_horizontal, basic_flip_vertical;
};

// Base class for Qt camera factories
class QtCameraFactory : public CameraFactory {
    std::unique_ptr<CameraInterface> CreatePreview(const std::string& config, int width, int height,
                                                   const Service::CAM::Flip& flip) override;
};

} // namespace Camera
