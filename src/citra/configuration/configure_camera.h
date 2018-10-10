// Copyright 2018 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "core/camera/factory.h"
#include "core/camera/interface.h"

namespace Ui {
class ConfigureCamera;
} // namespace Ui

class ConfigureCamera : public QWidget {
    Q_OBJECT

public:
    explicit ConfigureCamera(QWidget* parent = nullptr);
    ~ConfigureCamera();

    void applyConfiguration();

    void timerEvent(QTimerEvent*) override;

public slots:
    void setConfiguration();
    void onToolButtonClicked();

private:
    enum class CameraPosition { RearRight, Front, RearLeft, RearBoth, Null };
    static const std::array<std::string, 3> Implementations;

    /// Record the current configuration
    void recordConfig();

    /// Updates camera mode
    void updateCameraMode();

    /// Updates image source
    void updateImageSourceUI();

    void startPreviewing();
    void stopPreviewing();
    void connectEvents();
    CameraPosition getCameraSelection();
    int getSelectedCameraIndex();

private:
    std::unique_ptr<Ui::ConfigureCamera> ui;
    std::array<std::string, 3> camera_name;
    std::array<std::string, 3> camera_config;
    std::array<int, 3> camera_flip;
    int timer_id{};
    int preview_width{};
    int preview_height{};
    CameraPosition current_selected{CameraPosition::Front};
    bool is_previewing{};
    std::unique_ptr<Camera::CameraInterface> previewing_camera;
};
